//
// Created by Morten on 05-08-2020.
//

#include "AalWiNesBuilder.h"


namespace aalwines {

    namespace experimental {
        inline void from_json(const json & j, RoutingTable::action_t& action) {
            if (!j.is_object()){
                throw base_error("error: Operation is not an object");
            }
            int i = 0;
            for (const auto& [op_string, label_string] : j.items()) {
                if (i > 0){
                    throw base_error("error: Operation object must contain exactly one property");
                }
                i++;
                if (op_string == "pop") {
                    action._op = RoutingTable::op_t::POP;
                } else if (op_string == "swap") {
                    action._op = RoutingTable::op_t::SWAP;
                    action._op_label.set_value(label_string);
                } else if (op_string == "push") {
                    action._op = RoutingTable::op_t::PUSH;
                    action._op_label.set_value(label_string);
                } else {
                    std::stringstream es;
                    es << "error: Unknown operation: \"" << op_string << "\": \"" << label_string << "\"" << std::endl;
                    throw base_error(es.str());
                }
            }
            if (i == 0) {
                throw base_error("error: Operation object was empty");
            }
        }

        inline void to_json(json & j, const RoutingTable::action_t& action) {
            j = json::object();
            switch (action._op) {
                default: // TODO: Make RoutingTable::op_t enum class and remove this default:
                case RoutingTable::op_t::POP:
                    j["pop"] = "";
                    break;
                case RoutingTable::op_t::SWAP: {
                    std::stringstream label;
                    label << action._op_label; // TODO: Is this format correct?
                    j["swap"] = label.str();
                    break;
                }
                case RoutingTable::op_t::PUSH: {
                    std::stringstream label;
                    label << action._op_label; // TODO: Is this format correct?
                    j["push"] = label.str();
                    break;
                }
            }
        }
    }

    Network AalWiNesBuilder::parse(const json& json_network, std::ostream& warnings) {
        std::vector<std::unique_ptr<Router>> routers;
        std::vector<const Interface*> all_interfaces;
        Network::routermap_t mapping;

        std::stringstream es;

        auto network_name = json_network.at("name").get<std::string>();

        auto json_routers = json_network.at("routers");
        if (!json_routers.is_array()) {
            throw base_error("error: routers field is not an array.");
        }
        for (const auto& json_router : json_routers) {
            auto names = json_router.at("names").get<std::vector<std::string>>();
            if (names.empty()) {
                throw base_error("error: Router must have at least one name.");
            }
            size_t id = routers.size();

            auto loc_it = json_router.find("location");
            auto coordinate = loc_it != json_router.end() ? std::optional<Coordinate>(loc_it->get<Coordinate>()) : std::nullopt;
            routers.emplace_back(std::make_unique<Router>(id, names, coordinate));
            auto router = routers.back().get();
            for (const auto& name : names) {
                auto res = mapping.insert(name.c_str(), name.length());
                if(!res.first)
                {
                    es << "error: Duplicate definition of \"" << name << "\", previously found in entry " << mapping.get_data(res.second)->index() << std::endl;
                    throw base_error(es.str());
                }
                mapping.get_data(res.second) = router;
            }

            auto json_interfaces = json_router.at("interfaces");
            if (!json_interfaces.is_array()) {
                es << "error: interfaces field of router \"" << names[0] << "\" is not an array.";
                throw base_error(es.str());
            }
            // First create all interfaces.
            for (const auto& json_interface : json_interfaces) {
                auto interface_names = json_interface.at("names").get<std::vector<std::string>>();
                if (interface_names.empty()) {
                    es << "error: Interface of router \"" << names[0] << "\" must have at least one name.";
                    throw base_error(es.str());
                }
                router->add_interface(interface_names, all_interfaces);
            }
            // Then create routing tables for the interfaces.
            for (const auto& json_interface : json_interfaces) {
                auto interface_name = json_interface.at("names").get<std::vector<std::string>>()[0];

                auto json_routing_table = json_interface.at("routing_table");
                if (!json_routing_table.is_object()) {
                    es << "error: routing table field for interface \"" << interface_name << "\" of router \"" << names[0] << "\" is not an object.";
                    throw base_error(es.str());
                }

                auto interface = router->find_interface(interface_name);
                for (const auto& [label_string, json_routing_entries] : json_routing_table.items()) {
                    auto entry = interface->table().emplace_entry(RoutingTable::label_t(label_string));

                    if (!json_routing_entries.is_array()) {
                        es << "error: Value of routing table entry \"" << label_string << "\" is not an array. In interface \"" << interface_name << "\" of router \"" << names[0] << "\"." << std::endl;
                        throw base_error(es.str());
                    }
                    for (const auto& json_routing_entry : json_routing_entries) {
                        auto via = router->find_interface(json_routing_entry.at("out").get<std::string>());
                        auto ops = json_routing_entry.at("ops").get<std::vector<RoutingTable::action_t>>();
                        auto priority = json_routing_entry.at("priority").get<size_t>();
                        auto weight = json_routing_entry.contains("weight") ? json_routing_entry.at("weight").get<uint32_t>() : 0;
                        entry._rules.emplace_back(std::move(ops), via, priority, weight);
                    }
                }

            }
        }

        auto json_links = json_network.at("links");
        if (!json_links.is_array()) {
            throw base_error("error: links field is not an array.");
        }
        for (const auto& json_link : json_links) {
            auto temp_it = json_link.find("bidirectional");
            bool bidirectional = temp_it != json_link.end() && temp_it->get<bool>();
            auto from_interface = json_link.at("from_interface").get<std::string>();
            auto from_router = json_link.at("from_router").get<std::string>();
            auto to_interface = json_link.at("to_interface").get<std::string>();
            auto to_router = json_link.at("to_router").get<std::string>();


        }

        return Network(std::move(mapping), std::move(routers), std::move(all_interfaces));
    }

/*
    struct Link {
        bool bidirectional;
        std::string from_interface;
        std::string from_router;
        std::string to_interface;
        std::string to_router;
    };
    struct Network {
        std::vector<Link> links;
        std::string name;
        std::vector<Router> routers;
    };

    void from_json(const json & j, aalwines::Link & x);
    void to_json(json & j, const aalwines::Link & x);

    void from_json(const json & j, aalwines::Network & x);
    void to_json(json & j, const aalwines::Network & x);

    inline void from_json(const json & j, aalwines::Network& x) {
        x.links = j.at("links").get<std::vector<aalwines::Link>>();
        x.name = j.at("name").get<std::string>();
        x.routers = j.at("routers").get<std::vector<aalwines::Router>>();
    }

    inline void to_json(json & j, const aalwines::Network & x) {
        j = json::object();
        j["links"] = x.links;
        j["name"] = x.name;
        j["routers"] = x.routers;
    }

    inline void from_json(const json & j, aalwines::Link& x) {
        x.bidirectional = j.find("bidirectional") != j.end() && j.at("bidirectional").get<bool>();
        x.from_interface = j.at("from_interface").get<std::string>();
        x.from_router = j.at("from_router").get<std::string>();
        x.to_interface = j.at("to_interface").get<std::string>();
        x.to_router = j.at("to_router").get<std::string>();
    }

    inline void to_json(json & j, const aalwines::Link & x) {
        j = json::object();
        j["bidirectional"] = x.bidirectional;
        j["from_interface"] = x.from_interface;
        j["from_router"] = x.from_router;
        j["to_interface"] = x.to_interface;
        j["to_router"] = x.to_router;
    }


    inline void to_json(json & j, const aalwines::Interface & x) {
        j = json::object();
        j["names"] = x.names;
        j["routing_table"] = x.routing_table;
    }

    inline void from_json(const json & j, aalwines::Router& x) {
        x.interfaces = j.at("interfaces").get<std::vector<aalwines::Interface>>();
        x.location = aalwines::get_optional<aalwines::Location>(j, "location");
        x.names = j.at("names").get<std::vector<std::string>>();
    }

    inline void to_json(json & j, const aalwines::Router & x) {
        j = json::object();
        j["interfaces"] = x.interfaces;
        j["location"] = x.location;
        j["names"] = x.names;
    }
*/

}
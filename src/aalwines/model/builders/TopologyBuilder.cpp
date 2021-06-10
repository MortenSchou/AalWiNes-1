/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Copyright Dan Kristiansen and Morten K. Schou
 */

/*
 * File:   TopologyBuilder.cpp
 * Author: Dan Kristiansen
 *
 * Created on 04-06-2020.
 */

#include <fstream>
#include <sstream>
#include "TopologyBuilder.h"

namespace aalwines {

    // Stolen from: https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
    // trim from start (in place)
    static inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
    }
    // trim from end (in place)
    static inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }
    // trim from both ends (in place)
    static inline void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    Network aalwines::TopologyBuilder::parse(const std::string &gml, std::ostream& warnings) {
        std::ifstream file(gml);

        if(!file){
            std::cerr << "Error file not found." << std::endl;
        }

        std::vector<std::pair<size_t, size_t>> _parsed_links;
        std::vector<std::pair<std::string, std::optional<Coordinate>>> _all_routers;
        std::unordered_map<size_t, size_t> _index_map; // From 'id' attribute to its index in _all_routers vector.
        std::vector<std::vector<std::string>> _return_links;
        bool directed = false;

        std::string str;
        while (std::getline(file >> std::ws, str, ' ')) {
            trim(str);
            if (str == "directed") {
                char next;
                file >> next;
                if (next == '1') {
                    directed = true;
                }
            } else if (str =="node") {
                size_t id;
                std::string router_name;
                double latitude = 0;
                double longitude = 0;
                while (std::getline(file, str)){
                    trim(str);
                    auto pos = str.find(' ');
                    if (pos == str.size()) continue;
                    std::string key = str.substr(0, pos);
                    if (key == "id") {
                        id = std::stoul(str.substr(pos+1));
                    } else if (key == "label") {
                        router_name = str.substr(pos+2, str.size() - pos - 3);
                        trim(router_name);
                        //Get_interface dont handle ' ' very well
                        std::replace(router_name.begin(), router_name.end(), ' ', '_');
                        router_name.erase(std::remove_if(router_name.begin(), router_name.end(),
                                [](auto const& c) -> bool { return !std::isalnum(c) && c != '_' && c != '-'; }), router_name.end());
                    } else if (key == "name" && router_name.empty()) { // In some cases we have a 'name' attribute instead of a 'label' attribute.
                        router_name = str.substr(pos+1);
                        trim(router_name);
                        std::replace(router_name.begin(), router_name.end(), ' ', '_');
                        router_name.erase(std::remove_if(router_name.begin(), router_name.end(),
                                [](auto const& c) -> bool { return !std::isalnum(c) && c != '_' && c != '-'; }), router_name.end());
                    } else if (key == "Latitude") {
                        latitude = std::stod(str.substr(pos+1));
                    } else if (key == "Longitude") {
                        longitude = std::stod(str.substr(pos+1));
                    } else if (key == "]") {
                        if (router_name.empty()) {
                            router_name = std::to_string(id);
                        }
                        if (std::find_if(_all_routers.begin(), _all_routers.end(), // We may have duplicate names, so we find a suffix to make it unique.
                                         [&](const auto& item){ return item.first == router_name; }) != _all_routers.end()) {
                            size_t suffix = 2;
                            std::string new_router_name = router_name + std::to_string(suffix);
                            while (std::find_if(_all_routers.begin(), _all_routers.end(),
                                                [&](const auto& item){ return item.first == new_router_name; }) != _all_routers.end()) {
                                suffix++;
                                new_router_name = router_name + std::to_string(suffix);
                            }
                            router_name = new_router_name;
                        }
                        auto index = _all_routers.size();
                        if(latitude == 0 && longitude == 0) {
                            _all_routers.emplace_back(router_name, std::nullopt);
                        } else {
                            _all_routers.emplace_back(router_name, Coordinate{latitude, longitude});
                        }
                        _index_map.emplace(id, index);
                        _return_links.emplace_back();
                        break;
                    }
                }
            } else if (str == "edge") {
                size_t source;
                size_t target;
                while (std::getline(file, str, ' ')) {
                    if (str == "source") {
                        file >> source;
                    } else if (str == "target") {
                        file >> target;
                        _parsed_links.emplace_back(source, target);
                        break;
                    }
                }
            }
        }

        for(const auto& link : _parsed_links){
            auto from_id = _index_map[link.first];
            auto to_id = _index_map[link.second];
            _return_links[from_id].emplace_back(_all_routers[to_id].first);
            if (!directed) {
                _return_links[to_id].emplace_back(_all_routers[from_id].first);
            }
        }
        auto network = Network::make_network(_all_routers, _return_links);
        // Use filename (without file-extension) is network name.
        auto pos = gml.find_last_of('/');
        auto file_name = gml.substr((pos == std::string::npos) ? 0 : pos + 1);
        network.name = file_name.substr(0, file_name.find('.'));
        return network;
    }

    inline json to_json_no_routing(const Router& router) {
        auto j = json::object();
        j["name"] = router.names().back();
        if (router.names().size() > 1) {
            j["alias"] = json::array();
            for (size_t i = 0; i < router.names().size() - 1; ++i) {
                j["alias"].emplace_back(router.names()[i]);
            }
        }
        j["interfaces"] = json::array();
        j["interfaces"].emplace_back();
        j["interfaces"].back()["routing_table"] = json::object(); // Only topology, so empty routing_table in this mode.
        j["interfaces"].back()["names"] = json::array();
        for (const auto& interface : router.interfaces()) {
            j["interfaces"].back()["names"].push_back(interface->get_name());
        }
        if (router.coordinate()) {
            j["location"] = router.coordinate().value();
        }
        return j;
    }

    json TopologyBuilder::json_topology(const Network& network, bool no_routing) {
        auto j = json::object();
        j["name"] = network.name;
        j["routers"] = json::array();
        for (const auto& router : network.routers()) {
            if (router->is_null()) continue;
            if (no_routing) {
                j["routers"].push_back(to_json_no_routing(*router));
            } else {
                j["routers"].push_back(router);
            }
        }
        auto links_j = json::array();
        for (const auto& interface : network.all_interfaces()){
            if (interface->match() == nullptr) continue; // Not connected
            if (interface->source()->is_null() || interface->target()->is_null()) continue; // Skip the NULL router
            assert(interface->match()->match() == interface);
            bool bidirectional = interface->weight == interface->match()->weight && interface->bandwidth == interface->match()->bandwidth && interface->latency == interface->match()->latency;
            if (interface->global_id() > interface->match()->global_id() && bidirectional) continue; // Already covered by bidirectional link the other way.

            auto link_j = json::object();
            link_j["from_interface"] = interface->get_name();
            link_j["from_router"] = interface->source()->name();
            link_j["to_interface"] = interface->match()->get_name();
            link_j["to_router"] = interface->target()->name();
            if (bidirectional) {
                link_j["bidirectional"] = true;
            }
            if (interface->weight != std::numeric_limits<uint32_t>::max()) {
                link_j["weight"] = interface->weight;
            }
            if (interface->bandwidth != std::numeric_limits<uint32_t>::max()) {
                link_j["bandwidth"] = interface->bandwidth;
            }
            if (interface->latency != std::numeric_limits<uint32_t>::max()) {
                link_j["latency"] = interface->latency;
            }
            links_j.push_back(link_j);
        }
        j["links"] = links_j;
        return j;
    }

    std::string parse_to(std::string& s, char delimiter) {
        auto pos = s.find(delimiter);
        auto val = s.substr(0,pos);
        s.erase(0, pos + 1);
        return val;
    }
    std::string parse_to(std::string& s, const std::string& delimiter) {
        auto pos = s.find(delimiter);
        auto val = s.substr(0,pos);
        s.erase(0, pos + delimiter.length());
        return val;
    }

    Network TopologyBuilder::parse_aubry_graph(const std::string& input_file, std::ostream& warnings) {
        std::ifstream stream(input_file);
        if (!stream.is_open()) {
            std::stringstream es;
            es << "error: Could not open file : " << input_file << std::endl;
            throw base_error(es.str());
        }

        Network network;
        auto temp = input_file.substr(input_file.find_last_of('/')+1);
        network.name = temp.substr(0, temp.find_last_of('.'));

        std::string line;
        std::getline(stream, line);
        unsigned long num_nodes = std::stoul(line.substr(line.find(' ')+1));

        std::vector<std::string> names(num_nodes);

        std::getline(stream, line);
        assert(line == "label x y");
        for (unsigned long i = 0; i < num_nodes; i++) {
            std::getline(stream, line);
            unsigned long id = std::stoul(parse_to(line,'_'));
            //assert(id == i); // Not fulfilled, but i is the correct index.
            std::string name = parse_to(line,' ');
            if (name == "?") {
                name = "Unknown";
            }
            if (std::find_if(names.begin(), names.end(), // We may have duplicate names, so we find a suffix to make it unique.
                             [&name](const auto& item){ return item == name; }) != names.end()) {
                size_t suffix = 2;
                std::string new_name = name + std::to_string(suffix);
                while (std::find_if(names.begin(), names.end(),
                                    [&new_name](const auto& item){ return item == new_name; }) != names.end()) {
                    suffix++;
                    new_name = name + std::to_string(suffix);
                }
                name = new_name;
            }
            names[i] = name;
            double longitude = std::stod(parse_to(line,' '));
            double latitude = std::stod(line);
            std::optional<Coordinate> coordinate = (latitude == 0.0 && longitude == 0.0) ? std::nullopt : std::make_optional<Coordinate>(latitude, longitude);
            auto router = network.add_router(name, coordinate);
            router->emplace_table();
        }
        std::getline(stream, line);
        assert(line.empty());
        std::getline(stream, line);
        unsigned long num_edges = std::stoul(line.substr(line.find(' ')+1));
        std::getline(stream, line);
        assert(line == "label src dest weight bw delay");
        for (unsigned long i = 0; i < num_edges; i++) {
            std::getline(stream, line);
            auto e = parse_to(line,'_');
            assert(e == "edge");
            unsigned long id = std::stoul(parse_to(line,' '));
            assert(id == i);
            auto src_id = std::stoul(parse_to(line,' '));
            const std::string& src = names.at(src_id);
            auto src_router = network.find_router(src);
            assert(src_router != nullptr);
            auto dest_id = std::stoul(parse_to(line,' '));
            const std::string& dest = names.at(dest_id);
            auto dest_router = network.find_router(dest);
            assert(dest_router != nullptr);
            uint32_t weight = std::stoul(parse_to(line,' '));
            uint32_t bw = std::stoul(parse_to(line,' '));
            uint32_t delay = std::stoul(line);

            auto src_interface = network.add_interface_to(src_router, src_router->tables().back().get());
            auto dest_interface = network.add_interface_to(dest_router, dest_router->tables().back().get());
            src_interface->make_pairing(dest_interface);
            src_interface->weight = weight;
            src_interface->bandwidth = bw;
            src_interface->latency = delay;
            dest_interface->weight = weight;
            dest_interface->bandwidth = bw;
            dest_interface->latency = delay;

            i++;
            std::stringstream expected_next_line;
            expected_next_line << "edge_" << i << " " << dest_id << " " << src_id << " " << weight << " " << bw << " " << delay;
            std::getline(stream, line);
            assert(line == expected_next_line.str());
        }
        return network;
    }
}
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
 *  Copyright Peter G. Jensen
 */

/* 
 * File:   Network.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 17, 2019, 2:17 PM
 */

#include "Network.h"
#include "filter.h"

#include <cassert>
#include <map>

namespace aalwines {

    Router* Network::find_router(const std::string& router_name) {
        auto from_res = _mapping.exists(router_name);
        return from_res.first ? _mapping.get_data(from_res.second) : nullptr;
    }

    Router* Network::get_router(size_t id) {
        return id < _routers.size() ? _routers[id].get() : nullptr;
    }

    std::pair<bool, Interface*> Network::insert_interface_to(const std::string& interface_name, Router* router) {
        return router->insert_interface(interface_name, _all_interfaces);
    }
    std::pair<bool, Interface*> Network::insert_interface_to(const std::string& interface_name, const std::string& router_name) {
        auto router = find_router(router_name);
        return router == nullptr ? std::make_pair(false, nullptr) : insert_interface_to(interface_name, router);
    }

    void Network::add_null_router() {
        auto null_router = add_router("NULL", true);
        for(const auto& r : routers()) {
            for(const auto& inf : r->interfaces()) {
                if(inf->match() == nullptr) {
                    std::stringstream interface_name;
                    interface_name << "i" << inf->global_id();
                    insert_interface_to(interface_name.str(), null_router).second->make_pairing(inf.get());
                }
            }
        }
    }

    const char* empty_string = "";

    std::unordered_set<Query::label_t> Network::interfaces(filter_t& filter)
    {
        std::unordered_set<Query::label_t> res;
        for (auto& r : _routers) {
            if (filter._from(r->name().c_str())) {
                for (auto& i : r->interfaces()) {
                    if (i->is_virtual()) continue;
                    // can we have empty interfaces??
                    assert(i);
                    auto fname = r->interface_name(i->id());
                    std::string tname;
                    const char* tr = empty_string;
                    if (i->target() != nullptr) {
                        if (i->match() != nullptr) {
                            tname = i->target()->interface_name(i->match()->id());
                        }
                        tr = i->target()->name().c_str();
                    }
                    if (filter._link(fname.c_str(), tname.c_str(), tr)) {
                        res.insert(Query::label_t{Query::INTERFACE, 0, i->global_id()}); // TODO: little hacksy, but we have uniform types in the parser
                    }
                }
            }
        }
        return res;
    }

    void Network::move_network(Network&& nested_network){
        // Find NULL router
        auto null_router = _mapping["NULL"];

        // Move old network into new network.
        for (auto&& e : nested_network._routers) {
            if (e->is_null()) {
                continue;
            }
            // Find unique name for router
            std::string new_name = e->name();
            while(_mapping.exists(new_name).first){
                new_name += "'";
            }
            e->change_name(new_name);

            // Add interfaces to _all_interfaces and update their global id.
            for (auto&& inf: e->interfaces()) {
                inf->set_global_id(_all_interfaces.size());
                _all_interfaces.push_back(inf.get());
                // Transfer links from old NULL router to new NULL router.
                if (inf->target()->is_null()){
                    std::stringstream ss;
                    ss << "i" << inf->global_id();
                    insert_interface_to(ss.str(), null_router).second->make_pairing(inf.get());
                }
            }

            // Move router to new network
            e->set_index(_routers.size());
            _routers.emplace_back(std::move(e));
            _mapping[new_name] = _routers.back().get();
        }
    }

    void Network::inject_network(Interface* link, Network&& nested_network, Interface* nested_ingoing,
            Interface* nested_outgoing, RoutingTable::label_t pre_label, RoutingTable::label_t post_label) {
        assert(nested_ingoing->target()->is_null());
        assert(nested_outgoing->target()->is_null());
        assert(this->size());
        assert(nested_network.size());

        // Pair interfaces for injection and create virtual interface to filter post_label before POP.
        auto link_end = link->match();
        link->make_pairing(nested_ingoing);
        auto virtual_guard = insert_interface_to("__virtual_guard__", nested_outgoing->source()).second; // Assumes these names are unique for this router.
        auto nested_end_link = insert_interface_to("__end_link__", nested_outgoing->source()).second;
        nested_outgoing->make_pairing(virtual_guard);
        link_end->make_pairing(nested_end_link);

        move_network(std::move(nested_network));

        // Add push and pop rules.
        for (auto&& interface : link->source()->interfaces()) {
            interface->table().add_to_outgoing(link, {RoutingTable::op_t::PUSH, pre_label});
        }
        virtual_guard->table().add_rule(post_label, {RoutingTable::op_t::POP, RoutingTable::label_t{}}, nested_end_link);
    }

    void Network::concat_network(Interface* link, Network&& nested_network, Interface* nested_ingoing, RoutingTable::label_t post_label) {
        assert(nested_ingoing->target()->is_null());
        assert(link->target()->is_null());
        assert(this->size());
        assert(nested_network.size());

        move_network(std::move(nested_network));

        // Pair interfaces for concatenation.
        link->make_pairing(nested_ingoing);
    }

    void Network::print_dot(std::ostream& s)
    {
        s << "digraph network {\n";
        for (auto& r : _routers) {
            r->print_dot(s);
        }
        s << "}" << std::endl;
    }
    void Network::print_simple(std::ostream& s)
    {
        for(auto& r : _routers)
        {
            s << "router: \"" << r->name() << "\":\n";
            r->print_simple(s);
        }
    }
    void Network::print_json(std::ostream& s)
    {
        s << "\t\"routers\": {\n";
        bool first = true;
        for(auto& r : _routers)
        {
            if (r->is_null()) {
                continue;
            }
            if (first) {
                first = false;
            } else {
                s << ",\n";
            }            
            s << "\t\t\"" << r->name() << "\": {\n";
            r->print_json(s);
            s << "\t\t}";
        }
        s << "\n\t}";
    }

    void Network::write_prex_routing(std::ostream& s)
    {
        s << "<routes>\n";
        s << "  <routings>\n";
        for(auto& r : _routers)
        {
            if(r->is_null()) continue;
            
            // empty-check
            bool all_empty = true;
            for(auto& inf : r->interfaces())
                all_empty &= inf->table().empty();
            if(all_empty)
                continue;
            
            
            s << "    <routing for=\"" << r->name() << "\">\n";
            s << "      <destinations>\n";
            
            // make uniformly sorted output, easier for debugging
            std::vector<std::pair<std::string,Interface*>> sinfs;
            for(auto& inf : r->interfaces())
            {
                auto fname = r->interface_name(inf->id());
                sinfs.emplace_back(fname, inf.get());
            }
            std::sort(sinfs.begin(), sinfs.end(), [](auto& a, auto& b){
                return strcmp(a.first.c_str(), b.first.c_str()) < 0;
            });
            for(auto& sinf : sinfs)
            {
                auto inf = sinf.second;
                assert(std::is_sorted(inf->table().entries().begin(), inf->table().entries().end()));
                
                for(auto& e : inf->table().entries())
                {
                    assert(e._ingoing == nullptr || e._ingoing == inf);
                    s << "        <destination from=\"" << sinf.first << "\" ";
                    if((e._top_label.type() & Query::MPLS) != 0)
                    {
                        s << "label=\"";
                    }
                    else if((e._top_label.type() & (Query::IP4 | Query::IP6 | Query::ANYIP)) != 0)
                    {
                        s << "ip=\"";
                    }
                    else
                    {
                        assert(false);
                    }
                    s << e._top_label << "\">\n";
                    s << "          <te-groups>\n";
                    s << "            <te-group>\n";
                    s << "              <routes>\n";
                    auto cpy = e._rules;
                    std::sort(cpy.begin(), cpy.end(), [](auto& a, auto& b) {
                        return a._priority < b._priority;
                    });
                    auto ow = cpy.empty() ? 0 : cpy.front()._priority;
                    for(auto& rule : cpy)
                    {
                        if(rule._priority > ow)
                        {
                            s << "              </routes>\n";
                            s << "            </te-group>\n";
                            s << "            <te-group>\n";
                            s << "              <routes>\n";
                            ow = rule._priority;
                        }
                        assert(ow == rule._priority);
                        bool handled = false;
                        switch(rule._type)
                        {
                        case RoutingTable::DISCARD:
                            s << "                <discard/>\n";
                            handled = true;
                            break;
                        case RoutingTable::RECEIVE:
                            s << "                <receive/>\n";
                            handled = true;
                            break;
                        case RoutingTable::ROUTE:
                            s << "                <reroute/>\n";
                            handled = true;
                            break;
                        default:
                            break;
                        }
                        if(!handled)
                        {
                            assert(rule._via->source() == r.get());
                            auto tname = r->interface_name(rule._via->id());
                            s << "                <route to=\"" << tname << "\">\n";
                            s << "                  <actions>\n";
                            for(auto& o : rule._ops)
                            {
                                switch(o._op)
                                {
                                case RoutingTable::op_t::PUSH:
                                    s << "                    <action arg=\"" << o._op_label << "\" type=\"push\"/>\n";
                                    break;
                                case RoutingTable::op_t::POP:
                                    s << "                    <action type=\"pop\"/>\n";
                                    break;
                                case RoutingTable::op_t::SWAP:
                                    s << "                    <action arg=\"" << o._op_label << "\" type=\"swap\"/>\n";
                                    break;
                                }
                            }
                            s << "                  </actions>\n";
                            s << "                </route>\n";
                        }
                        else
                        {
                            break;
                        }
                    }
                    s << "              </routes>\n";
                    s << "            </te-group>\n";
                    s << "          </te-groups>\n";
                    s << "        </destination>\n";
                }
            }
            s << "      </destinations>\n";
            s << "    </routing>\n";
        }
        s << "  </routings>\n";
        s << "</routes>\n";
    }

    void Network::write_prex_topology(std::ostream& s)
    {
        s << "<network>\n  <routers>\n";
        for(auto& r : _routers)
        {
            if(r->is_null()) continue;
            if(r->interfaces().empty()) continue;
            s << "    <router name=\"" << r->name() << "\">\n";
            s << "      <interfaces>\n";
            for(auto& inf : r->interfaces())
            {
                auto fname = r->interface_name(inf->id());
                s << "        <interface name=\"" << fname << "\"/>\n";
            }
            s << "      </interfaces>\n";
            s << "    </router>\n";
        }
        s << "  </routers>\n  <links>\n";
        for(auto& r : _routers)
        {
            if(r->is_null()) continue;
            for(auto& inf : r->interfaces())
            {
                if(inf->source()->index() > inf->target()->index())
                    continue;
                
                if(inf->source()->is_null()) continue;
                if(inf->target()->is_null()) continue;
                
                auto fname = r->interface_name(inf->id());
                auto oname = inf->target()->interface_name(inf->match()->id());
                s << "    <link>\n      <sides>\n" <<
                     "        <shared_interface interface=\"" << fname <<
                     "\" router=\"" << inf->source()->name() << "\"/>\n" <<
                     "        <shared_interface interface=\"" << oname <<
                     "\" router=\"" << inf->target()->name() << "\"/>\n" <<
                     "      </sides>\n    </link>\n";
            }
        }
        s << "  </links>\n</network>\n";
    }

    Network Network::make_network(const std::vector<std::string>& names, const std::vector<std::vector<std::string>>& links){
        Network network;
        for (size_t i = 0; i < names.size(); ++i) {
            auto name = names[i];
            auto router = network.add_router(name);
            network.insert_interface_to("i" + name, router);
            for (const auto& other : links[i]) {
                network.insert_interface_to(other, router);
            }
        }
        for (size_t i = 0; i < names.size(); ++i) {
            auto name = names[i];
            for (const auto &other : links[i]) {
                auto router1 = network.find_router(name);
                assert(router1 != nullptr);
                auto router2 = network.find_router(other);
                if(router2 == nullptr) continue;
                router1->find_interface(other)->make_pairing(router2->find_interface(name));
            }
        }
        network.add_null_router();
        return network;
    }

}

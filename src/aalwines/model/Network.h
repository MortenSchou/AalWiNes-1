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
 * File:   Network.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 17, 2019, 2:17 PM
 */

#ifndef NETWORK_H
#define NETWORK_H
#include "Router.h"
#include "RoutingTable.h"
#include "Query.h"
#include "filter.h"

#include <ptrie/ptrie_map.h>
#include <vector>
#include <memory>
#include <functional>

namespace aalwines {
class Network {
public:
    struct data_flow {
        Interface* ingoing;
        Interface* outgoing;
        RoutingTable::label_t pre_label;
        RoutingTable::label_t post_label;
        data_flow(Interface* ingoing, Interface* outgoing, RoutingTable::label_t pre_label, RoutingTable::label_t post_label)
                : ingoing(ingoing), outgoing(outgoing), pre_label(pre_label), post_label(post_label) {
            assert(ingoing->target()->is_null());
            assert(outgoing->target()->is_null());
        }
        data_flow(std::pair<Interface*, RoutingTable::label_t> start, std::pair<Interface*, RoutingTable::label_t> end)
                : ingoing(start.first), outgoing(end.first), pre_label(start.second), post_label(end.second) {
            assert(ingoing->target()->is_null());
            assert(outgoing->target()->is_null());
        }
    };

public:
    using routermap_t = ptrie::map<char, Router*>;
    Network(routermap_t&& mapping, std::vector<std::unique_ptr < Router>>&& routers, std::vector<const Interface*>&& all_interfaces);

    Router *get_router(size_t id);
    const std::vector<std::unique_ptr<Router>>& get_all_routers() const { return _routers; }

    void inject_network(const std::vector<Interface*>& links, Network&& nested_network, const std::vector<data_flow>& flows);
    void concat_network(const std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>>& end_links,
            Network&& other_network, const std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>>& start_links);
    void concat_network(const std::vector<Interface*>& end_links, Network&& other_network, const std::vector<Interface*>& start_links);
    void inject_network(Interface* link, Network&& nested_network, Interface* nested_ingoing,
                        Interface* nested_outgoing, RoutingTable::label_t pre_label, RoutingTable::label_t post_label);
    void concat_network(Interface* end_link, Network&& other_network, Interface* start_link, RoutingTable::label_t unused);
    size_t size() const { return _routers.size(); }

    std::unordered_set<Query::label_t> interfaces(filter_t& filter);
    std::unordered_set<Query::label_t> get_labels(uint64_t label, uint64_t mask, Query::type_t type, bool exact = false);
    std::unordered_set<Query::label_t> all_labels();
    const std::vector<const Interface*>& all_interfaces() const { return _all_interfaces; }
    void print_dot(std::ostream& s);
    void print_dot_undirected(std::ostream& s);
    void print_simple(std::ostream& s);
    void print_json(std::ostream& s);
    bool is_service_label(const Query::label_t&) const;
    void write_prex_topology(std::ostream& s);
    void write_prex_routing(std::ostream& s);
    void write_stats(std::ostream &ofstream);
    size_t _pda_states = 0;
    size_t _pda_states_after_reduction = 0;
    size_t _pda_rules = 0;
    size_t _pda_rules_after_reduction = 0;

    static Network construct_synthetic_network(size_t nesting = 1);
    static Network make_network(const std::vector<std::string>& names, const std::vector<std::vector<std::string>>& links);
    static Network make_network(const std::vector<std::pair<std::string,Coordinate>>& names, const std::vector<std::vector<std::string>>& links);
private:
    // NO TOUCHEE AFTER INIT!
    routermap_t _mapping;
    std::vector<std::unique_ptr<Router>> _routers;
    size_t _total_labels = 0;
    std::vector<const Interface*> _all_interfaces;
    std::unordered_set<Query::label_t> _label_cache;
    std::unordered_set<Query::label_t> _non_service_label;
    void move_network(Network&& nested_network);
};
}

#endif /* NETWORK_H */


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
 *  Copyright Morten K. Schou
 */

/* 
 * File:   AnalyseNetwork.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 12-10-2020.
 */

#include <aalwines/model/builders/NetworkParsing.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <cassert>
#include <boost/functional/hash.hpp>

using namespace aalwines;


std::stack<Query::label_t> apply_ops(const std::vector<RoutingTable::action_t>& ops, const std::stack<Query::label_t>& label_stack) {
    std::stack<Query::label_t> new_stack{label_stack};
    for (const auto& operation : ops) {
        switch (operation._op) {
            case RoutingTable::op_t::POP:
                if (new_stack.empty()) {
                    return new_stack;
                }
                new_stack.pop();
                break;
            case RoutingTable::op_t::SWAP:
                if (new_stack.empty()) {
                    return new_stack;
                }
                new_stack.top() = operation._op_label;
                break;
            case RoutingTable::op_t::PUSH:
                new_stack.push(operation._op_label);
                break;
        }
    }
    return new_stack;
}

void search_all_paths(const std::vector<const Interface*>& stack, const std::stack<Query::label_t>& label_stack, std::map<std::vector<const Interface*>, size_t>& paths, std::map<std::vector<const Router*>, size_t>& router_paths, bool& progress) {
    if (stack.size() > 50) return; // Break infinite recursion caused by forwarding loops.
    for (const auto& entry : stack.back()->match()->table().entries()) {
        if (!entry._top_label.overlaps(label_stack.top())) continue;
        for (const auto& rule : entry._rules) {
            if (rule._type != RoutingTable::MPLS || rule._priority > 1) continue; // Only use MPLS rules. The other types are undocumented, and I don't know what they do.
            if (rule._via->target()->is_null()) {
                std::vector<const Interface*> new_stack;
                new_stack.reserve(stack.size() + 1);
                std::copy(stack.begin(), stack.end(), std::back_inserter(new_stack));
                new_stack.push_back(rule._via);
                ++paths[new_stack];
                std::vector<const Router*> router_path;
                router_path.reserve(stack.size());
                for (size_t i = 1; i < new_stack.size(); ++i) {
                    router_path.push_back(new_stack[i]->source());
                }
                ++router_paths[router_path];
                progress = true;
            } else {
                auto new_label_stack = apply_ops(rule._ops, label_stack);
                if (!new_label_stack.empty()) {
                    std::vector<const Interface*> new_stack;
                    new_stack.reserve(stack.size() + 1);
                    std::copy(stack.begin(), stack.end(), std::back_inserter(new_stack));
                    new_stack.push_back(rule._via);
                    search_all_paths(new_stack, new_label_stack, paths, router_paths, progress);
                }
            }
        }
    }
}


void analyse_paths(const Network& network) {
    std::map<std::vector<const Interface*>, size_t> paths;
    std::map<std::vector<const Router*>, size_t> router_paths;
    size_t fails = 0;
    for (const auto& router : network.routers()) {
        for (const auto& interface : router->interfaces()) {
            if (interface->target()->is_null()){
                for (const auto& entry : interface->table().entries()) {
                    bool progress = false;
                    std::stack<Query::label_t> label_stack;
                    if (!entry._top_label.overlaps(Query::label_t::any_ip)) {
                        label_stack.push(Query::label_t::any_ip);
                    }
                    label_stack.push(entry._top_label);
                    search_all_paths(std::vector<const Interface*>{interface->match()}, label_stack, paths, router_paths, progress);
                    if (!progress) {
                        ++fails;
                    }
                }
            }
        }
    }

    // Count occurrences and distinct paths between each pair of interfaces.
    std::unordered_map<std::pair<const Interface*, const Interface*>, std::pair<size_t,size_t>, boost::hash<std::pair<const Interface*, const Interface*>>> path_counts;
    for (const auto& [path, count] : paths) {
        if (path.front()->match() != path.back()) {
            auto& [c, d] = path_counts[std::make_pair(path.front()->match(), path.back())];
            c += count;
            ++d;
        }
    }

    // Print for each interface pair
    //for (const auto& [pair, counts] : path_counts) {
    //    std::cout << pair.first->get_name() << " -> " << pair.second->get_name() << " : " << counts.first << " , " << counts.second << std::endl;
    //}

    // Aggregate interface pairs to router pairs
    std::unordered_map<std::pair<const Router*, const Router*>, std::tuple<size_t,size_t,std::vector<std::vector<const Router*>>>, boost::hash<std::pair<const Router*, const Router*>>> router_path_counts;
    for (const auto& [pair, counts] : path_counts) {
        auto& [c, d, ps] = router_path_counts[std::make_pair(pair.first->source(), pair.second->source())];
        c += counts.first;
        d += counts.second;
    }
    // Store distinct router paths for each router pair.
    for (const auto& [path, count] : router_paths) {
        if (router_path_counts.count(std::make_pair(path.front(), path.back())) > 0) {
            auto& [c, d, ps] = router_path_counts[std::make_pair(path.front(), path.back())];
            ps.push_back(path);
        }
    }

    // Sort by router names for prettier output.
    std::vector<std::tuple<std::string, std::string, size_t, size_t, std::vector<std::vector<const Router*>>>> router_pair_counts;
    router_pair_counts.reserve(router_path_counts.size());
    for (const auto& [pair, count] : router_path_counts) {
        router_pair_counts.emplace_back(pair.first->name(), pair.second->name(), std::get<0>(count), std::get<1>(count), std::get<2>(count));
    }
    std::sort(router_pair_counts.begin(), router_pair_counts.end());

    // Output: source router -> target router : paths , distinct paths , [actual, routers, in, path]; [other, router, path];
    std::cout << std::endl << "*** ROUTERS ***" << std::endl;
    for (const auto& [s, t, count, distinct, r_paths] : router_pair_counts) {
        std::cout << s << " -> " << t << " : " << count << " , " << distinct << " , ";
        for (const auto& r_path : r_paths) {
            std::cout << "[";
            bool first = true;
            for (const auto& r : r_path) {
                if (first) {
                    first = false;
                } else {
                    std::cout << ", ";
                }
                std::cout << r->name();
            }
            std::cout << "]; ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl << "(label, ingress-interface)-pairs that did not have swap-path to egress-interface: " << fails << std::endl;

}

int main(int argc, const char** argv) {
    namespace po = boost::program_options;
    po::options_description opts;
    opts.add_options()
            ("help,h", "produce help message");

    NetworkParsing parser{"Input Options"};
    bool no_parser_warnings = false;

    auto options = parser.options();
    options.add_options()
        ("disable-parser-warnings,W", po::bool_switch(&no_parser_warnings), "Disable warnings from parser.");
    opts.add(options);
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }

    auto network = parser.parse(no_parser_warnings);

    std::cout << "Finding all paths..." << std::endl;
    analyse_paths(network);

}
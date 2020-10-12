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

#include <aalwines/model/builders/AalWiNesBuilder.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <cassert>
#include <boost/functional/hash.hpp>

using namespace aalwines;


const Interface* search_path_target(const Interface* out_interface, const Query::label_t& top_label) {
    for (const auto& entry : out_interface->match()->table().entries()) {
        if (entry._top_label != top_label) continue;
        for (const auto& rule : entry._rules) {
            if (rule._via->target()->is_null()) {
                return rule._via;
            } else if (!rule._ops.empty() && rule._ops.back()._op == RoutingTable::op_t::SWAP) {
                if (auto res = search_path_target(rule._via, rule._ops.back()._op_label)) {
                    return res;
                }
            }
        }
    }
    return nullptr;
}
void search_path_all(const Interface* out_interface, const Query::label_t& top_label, std::unordered_map<const Interface*, size_t>& targets) {
    for (const auto& entry : out_interface->match()->table().entries()) {
        if (entry._top_label != top_label) continue;
        for (const auto& rule : entry._rules) {
            if (rule._via->target()->is_null()) {
                ++targets[rule._via];
            } else if (!rule._ops.empty() && rule._ops.back()._op == RoutingTable::op_t::SWAP) {
                search_path_all(rule._via, rule._ops.back()._op_label, targets);
            }
        }
    }
}

template <bool all=false>
void analyse(const Network& network) {
    // Find input/output pairs with a swap path.
    size_t fails = 0;
    std::unordered_map<std::pair<const Interface*, const Interface*>, size_t, boost::hash<std::pair<const Interface*, const Interface*>>> pairs;
    for (const auto& router : network.routers()) {
        for (const auto& interface : router->interfaces()) {
            if (interface->target()->is_null()){
                for (const auto& entry : interface->table().entries()) {
                    if constexpr (all) {
                        std::unordered_map<const Interface*, size_t> targets;
                        search_path_all(interface->match(), entry._top_label, targets);
                        if (targets.empty()) {
                            ++fails;
                        } else {
                            for (const auto& [target, count] : targets) {
                                if (target != interface.get()) {
                                    pairs[std::make_pair(interface.get(), target)] += count;
                                }
                            }
                        }
                    } else {
                        if (auto target = search_path_target(interface->match(), entry._top_label)) {
                            if (target != interface.get()) {
                                ++pairs[std::make_pair(interface.get(), target)];
                            }
                        } else {
                            ++fails;
                        }
                    }
                }
            }
        }
    }

    std::cout << "Fails: " << fails << std::endl;

    // Do stuff
    for (const auto& [pair, count] : pairs) {
        std::cout << pair.first->get_name() << " -> " << pair.second->get_name() << " : " << count << std::endl;
    }

    std::unordered_map<std::pair<const Router*, const Router*>, size_t, boost::hash<std::pair<const Router*, const Router*>>> router_pairs;
    for (const auto& [pair, count] : pairs) {
        router_pairs[std::make_pair(pair.first->source(), pair.second->source())] += count;
    }
    std::vector<std::tuple<std::string, std::string, size_t>> router_pair_counts;
    router_pair_counts.reserve(router_pairs.size());
    for (const auto& [pair, count] : router_pairs) {
        router_pair_counts.emplace_back(pair.first->name(), pair.second->name(), count);
    }
    std::sort(router_pair_counts.begin(), router_pair_counts.end());

    std::cout << std::endl << "*** ROUTERS ***" << std::endl;
    for (const auto& [s, t, count] : router_pair_counts) {
        std::cout << s << " -> " << t << " : " << count << std::endl;
    }
}

int main(int argc, const char** argv) {
    namespace po = boost::program_options;
    po::options_description opts;
    opts.add_options()
            ("help,h", "produce help message");
    po::options_description input("Input Options");
    std::string json_file;
    bool no_parser_warnings = false;
    input.add_options()
            ("input", po::value<std::string>(&json_file),
             "An json-file defining the network in the AalWiNes MPLS Network format")
            ("disable-parser-warnings,W", po::bool_switch(&no_parser_warnings), "Disable warnings from parser.")
            ;
    opts.add(input);
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }

    if(json_file.empty())
    {
        std::cerr << "--input must be specified" << std::endl;
        exit(-1);
    }

    std::stringstream dummy;
    std::ostream& warnings = no_parser_warnings ? dummy : std::cerr;
    auto network = AalWiNesBuilder::parse(json_file, warnings);

    std::cout << "Searching for all paths..." << std::endl;
    analyse<true>(network);
    std::cout << std::endl << "Searching for one path per (interface,label)..." << std::endl;
    analyse<false>(network);

}
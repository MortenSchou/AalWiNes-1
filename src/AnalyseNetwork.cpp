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

#include <aalwines/model/builders/JuniperBuilder.h>
#include <aalwines/model/builders/PRexBuilder.h>
#include <aalwines/model/builders/AalWiNesBuilder.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <cassert>
#include <boost/functional/hash.hpp>

using namespace aalwines;

void search_all_paths(const std::vector<const Interface*>& stack, const Query::label_t top_label, std::map<std::vector<const Interface*>, size_t>& paths, std::map<std::vector<const Router*>, size_t>& router_paths, bool& progress) {
    for (const auto& entry : stack.back()->match()->table().entries()) {
        if (!entry._top_label.overlaps(top_label)) continue;
        for (const auto& rule : entry._rules) {
            if (rule._type != RoutingTable::MPLS) continue; // Only use MPLS rules. The other types are undocumented, and I don't know what they do.
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
            } else if (!rule._ops.empty() && rule._ops.back()._op == RoutingTable::op_t::SWAP) {
                std::vector<const Interface*> new_stack;
                new_stack.reserve(stack.size() + 1);
                std::copy(stack.begin(), stack.end(), std::back_inserter(new_stack));
                new_stack.push_back(rule._via);
                search_all_paths(new_stack, rule._ops.back()._op_label, paths, router_paths, progress);
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
                    search_all_paths(std::vector<const Interface*>{interface->match()}, entry._top_label, paths, router_paths, progress);
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
    po::options_description input("Input Options");
    std::string json_file;
    bool no_parser_warnings = false;
    std::string junos_config, prex_topo, prex_routing;
    bool skip_pfe = false;
    input.add_options()
            ("input", po::value<std::string>(&json_file),
             "An json-file defining the network in the AalWiNes MPLS Network format")
            ("disable-parser-warnings,W", po::bool_switch(&no_parser_warnings), "Disable warnings from parser.")
            ("juniper", po::value<std::string>(&junos_config),
             "A file containing a network-description; each line is a router in the format \"name,alias1,alias2:adjacency.xml,mpls.xml,pfe.xml\". ")
            ("topology", po::value<std::string>(&prex_topo),
             "An xml-file defining the topology in the P-Rex format")
            ("routing", po::value<std::string>(&prex_routing),
             "An xml-file defining the routing in the P-Rex format")
            ("skip-pfe", po::bool_switch(&skip_pfe),
             "Skip \"indirect\" cases of juniper-routing as package-drops (compatability with P-Rex semantics).")
            ;
/*  input.add_options()
            ("input", po::value<std::string>(&json_file),
             "An json-file defining the network in the AalWiNes MPLS Network format")
            ("disable-parser-warnings,W", po::bool_switch(&no_parser_warnings), "Disable warnings from parser.")
            ;*/

    opts.add(input);
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }

    if(!json_file.empty() && (!prex_routing.empty() || !prex_topo.empty() || !junos_config.empty()))
    {
        std::cerr << "--input cannot be used with --junos or --topology or --routing." << std::endl;
        exit(-1);
    }

    if(!junos_config.empty() && (!prex_routing.empty() || !prex_topo.empty()))
    {
        std::cerr << "--junos cannot be used with --topology or --routing." << std::endl;
        exit(-1);
    }

    if(prex_routing.empty() != prex_topo.empty())
    {
        std::cerr << "Both --topology and --routing have to be non-empty." << std::endl;
        exit(-1);
    }

    if(junos_config.empty() && prex_routing.empty() && prex_topo.empty() && json_file.empty())
    {
        std::cerr << "Either a Junos configuration or a P-Rex configuration or an AalWiNes json configuration must be given." << std::endl;
        exit(-1);
    }

    if(skip_pfe && junos_config.empty())
    {
        std::cerr << "--skip-pfe is only avaliable for --junos configurations." << std::endl;
        exit(-1);
    }

    std::stringstream dummy;
    std::ostream& warnings = no_parser_warnings ? dummy : std::cerr;

    auto network = junos_config.empty() ? (json_file.empty() ?
                   PRexBuilder::parse(prex_topo, prex_routing, warnings) :
                   AalWiNesBuilder::parse(json_file, warnings)) :
                   JuniperBuilder::parse(junos_config, warnings, skip_pfe);

    /*
    if(json_file.empty())
    {
        std::cerr << "--input must be specified" << std::endl;
        exit(-1);
    }*/
    // auto network = AalWiNesBuilder::parse(json_file, warnings);

    std::cout << "Finding all paths..." << std::endl;
    analyse_paths(network);

}
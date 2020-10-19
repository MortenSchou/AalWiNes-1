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
 * File:   GenerateRouting.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 19-10-2020.
 */

#include <aalwines/synthesis/RouteConstruction.h>
#include <aalwines/model/builders/NetworkParsing.h>
#include <aalwines/model/builders/AalWiNesBuilder.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace aalwines;

int main(int argc, const char** argv) {
    namespace po = boost::program_options;
    po::options_description opts;
    opts.add_options()
            ("help,h", "produce help message");

    NetworkParsing parser{"Options"};
    bool no_parser_warnings = false;
    size_t s_paths = 1, i_paths = 1;
    std::string json_destination;
    auto options = parser.options();
    options.add_options()
            ("disable-parser-warnings,W", po::bool_switch(&no_parser_warnings), "Disable warnings from parser.")
            ("service-label-paths,S", po::value<size_t>(&s_paths), "Number of service-label transit paths per outbound interface pair.")
            ("ip-paths,I", po::value<size_t>(&i_paths), "Number of IP tunneling paths per outbound interface pair.")
            ("write-json", po::value<std::string>(&json_destination), "Write the network in the AalWiNes MPLS Network format to the given file.")
            ;
    opts.add(options);
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }

    auto network = parser.parse(no_parser_warnings);

    std::cout << "Network-parsing-time: " << parser.duration() << std::endl << std::endl;

    std::cout << "Generating routes..." << std::endl;
    uint64_t i = 42;
    auto next_label = [&i](Query::type_t type = Query::type_t::MPLS){
        if (type == Query::type_t::ANYIP) {
            return Query::label_t::any_ip;
        } else {
            return Query::label_t(type, 0, i++);
        }
    };
    auto cost = [](const Interface* interface){ return interface->source()->coordinate() && interface->target()->coordinate() ?
                                                       interface->source()->coordinate()->distance_to(interface->target()->coordinate().value()) : 1 ; };
    std::vector<std::pair<Router*, Router*>> unlinked_routers;
    for(auto &r : network.routers()){
        if(r->is_null()) continue;
        for(auto &r_p : network.routers()){
            if (r == r_p || r_p->is_null()) continue;
            for (const auto& from_interface : r->interfaces()) {
                if (!from_interface->target()->is_null()) continue;
                for (const auto& to_interface : r_p->interfaces()) {
                    if (!to_interface->target()->is_null()) continue;
                    auto success = RouteConstruction::make_data_flow(from_interface.get(), to_interface.get(), s_paths, i_paths, next_label, cost);
                    if(!success){
                        unlinked_routers.emplace_back(std::make_pair(r.get(), r_p.get()));
                    }
                }
            }
        }
    }
    for(auto& inf : network.all_interfaces()){
        if (inf->target()->is_null() || inf->source()->is_null()) continue;
        RouteConstruction::make_reroute(inf, next_label, cost);
    }

    if (!json_destination.empty()) {
        std::ofstream out(json_destination);
        if(out.is_open()) {
            auto j = json::object();
            j["network"] = network;
            out << j << std::endl;
        } else {
            std::cerr << "Could not open --write-json\"" << json_destination << "\" for writing" << std::endl;
            exit(-1);
        }
    }

}
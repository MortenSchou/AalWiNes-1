//
// Created by Morten on 16-04-2020.
//

#include <aalwines/model/builders/TopologyZooBuilder.h>
#include <aalwines/model/Network.h>
#include <aalwines/synthesis/FastRerouting.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;
using namespace aalwines;

void make_queries(const Network& network, const size_t k, std::ostream& s){
    // TODO: Come up with good queries

    // Type 1
    s << "<.> ";
    s << ".";
    s << " <.> " << k << " DUAL" << std::endl;

    // Type 2
    s << "<.> ";
    s << ".";
    s << " <.> " << k << " DUAL" << std::endl;

    // Type 3
    s << "<.> ";
    s << ".";
    s << " <.> " << k << " DUAL" << std::endl;

    // Type 4
    s << "<.> ";
    s << ".";
    s << " <.> " << k << " DUAL" << std::endl;

    // Type 5
    s << "<.> ";
    s << ".";
    s << " <.> " << k << " DUAL" << std::endl;
}

int main(int argc, const char** argv) {
    po::options_description opts;
    opts.add_options()
            ("help,h", "produce help message");
    po::options_description input("Input Options");

    std::string topo_zoo;
    input.add_options()
            ("zoo,z", po::value<std::string>(&topo_zoo),"A gml-file defining the topology in the format from topology zoo")
            ;
    opts.add(input);
    size_t size = 1;
    size_t max_k = 3;
    bool dot_graph = false;
    bool print_simple = false;
    po::options_description generate("Test Options");
    generate.add_options()
            ("size,N", po::value<size_t>(&size), "the size variable (N)")
            ("max_k,k", po::value<size_t>(&max_k), "the maximal number of failures (k) for the queries generated")
            ("dot,d", po::bool_switch(&dot_graph), "print dot graph output")
            ("print_simple,p", po::bool_switch(&print_simple), "print simple routing output")
            ;
    opts.add(generate);
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }
    if(topo_zoo.empty()) {
        std::cerr << "Please provide topology zoo input file" << std::endl;
        exit(-1);
    }
    std::string name;
    size_t dot_pos = topo_zoo.find_last_of('.');
    if (dot_pos > 0 && topo_zoo.substr(dot_pos + 1) =="gml") {
        name = topo_zoo.substr(0, dot_pos);
    } else {
        name = topo_zoo;
    }

    auto input_network = TopologyZooBuilder::parse(topo_zoo);

    // TODO: Make_Larger(N)
    auto network = std::move(input_network);

    // Construct routes on network
    uint64_t i = 42;
    auto next_label = [&i](){return Query::label_t(Query::type_t::MPLS, 0, i++);};
    // TODO: Consider if cost=distance works for this??
    auto cost = [](const Interface* interface){
        return interface->source()->coordinate() && interface->target()->coordinate() ?
        interface->source()->coordinate()->distance_to(interface->target()->coordinate().value()) : 1 ; };
    // Make data flow for all pairs of routers.
    for(auto &r : network.get_all_routers()){
        if(r->is_null()) continue;
        for(auto &r_p : network.get_all_routers()){
            if (r == r_p || r_p->is_null()) continue;
            FastRerouting::make_data_flow(r->get_null_interface(), r_p->get_null_interface(), next_label, cost);
        }
    }
    // Make reroute for all interfaces
    for(auto& inf : network.all_interfaces()){
        if (inf->target()->is_null() || inf->source()->is_null()) continue;
        FastRerouting::make_reroute(inf, next_label, cost);
    }

    if (dot_graph) {
        network.print_dot_undirected(std::cout);
    }
    if (print_simple) {
        network.print_simple(std::cout);
    }

    std::stringstream queries;
    for (size_t k = 0; k <= max_k; ++k) {
        make_queries(network, k, queries);
    }

    auto topo_file = name + "-" + std::to_string(size) + "-topo.xml";
    std::ofstream out_topo(topo_file);
    if (out_topo.is_open()) {
        network.write_prex_topology(out_topo);
    } else {
        std::cerr << "Could not open file " << topo_file << " for writing" << std::endl;
        exit(-1);
    }
    auto routing_file = name + "-" + std::to_string(size) + "-routing.xml";
    std::ofstream out_route(routing_file);
    if (out_route.is_open()) {
        network.write_prex_routing(out_route);
    } else {
        std::cerr << "Could not open file " << routing_file << " for writing" << std::endl;
        exit(-1);
    }
    auto query_file = name + "-" + std::to_string(size) + "-queries.q";
    std::ofstream out_query(query_file);
    if (out_query.is_open()) {
        out_query << queries.rdbuf();
    } else {
        std::cerr << "Could not open file " << query_file << " for writing" << std::endl;
        exit(-1);
    }
}
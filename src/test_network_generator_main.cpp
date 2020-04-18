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

void make_query(const Network& network, const size_t type, const size_t k, std::ostream& s){
    // TODO: Come up with good queries
    switch (type) {
        default:
        case 1:
            // Type 1: Find a single step, single stack-size.
            s << "<.> . <.> " << k << " DUAL" << std::endl;
            break;
        case 2:
            // Type 2
            s << "<.> ";
            s << ".";
            s << " <.> " << k << " DUAL" << std::endl;
            break;
        case 3:
            // Type 3
            s << "<.> ";
            s << ".";
            s << " <.> " << k << " DUAL" << std::endl;
            break;
        case 4:
            // Type 4
            s << "<.> ";
            s << ".";
            s << " <.> " << k << " DUAL" << std::endl;
            break;
        case 5:
            // Type 5
            s << "<.> ";
            s << ".";
            s << " <.> " << k << " DUAL" << std::endl;
            break;
    }
}

Network make_large(const std::function<Network()>& make_base, size_t n) { // Use parse function, since Network doesn't currently have a copy-constructor.
    assert(n != 0);
    if (n == 1) {
        return make_base();
    }
    auto net = make_base();
    auto size = net.size() - 1; // Don't count NULL router.
    std::vector<Interface*> from_interfaces;
    for (size_t r = 0; r < size; r+=3) {
        from_interfaces.push_back(net.get_router(r)->get_null_interface());
    }
    for (size_t i = 1; i < n; ++i) {
        auto new_net = make_base();
        std::vector<Interface*> to_interfaces;
        std::vector<Interface*> temp_interfaces;
        for (size_t r = (i-1)%3; r < size; r+=3) {
            to_interfaces.push_back(new_net.get_router(r)->get_null_interface());
        }
        for (size_t r = i%3; r < size; r+=3) {
            temp_interfaces.push_back(new_net.get_router(r)->get_null_interface());
        }
        net.concat_network(from_interfaces, std::move(new_net), to_interfaces);
        from_interfaces = temp_interfaces;
    }
    return net;
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

    auto network = make_large([&topo_zoo](){ return TopologyZooBuilder::parse(topo_zoo); }, size);

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
            Interface *i1, *i2;
            if (r == r_p || r_p->is_null() ||
                (i1 = r->get_null_interface()) == nullptr || (i2 = r_p->get_null_interface()) == nullptr) continue;
            FastRerouting::make_data_flow(i1, i2, next_label, cost);
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

    for (size_t k = 0; k <= max_k; ++k) {
        for (size_t type = 1; type <= 5; ++type) {
            std::stringstream queries;
            make_query(network, type, k, queries);
            auto query_file = name + "-" + std::to_string(size) + "-Q" + std::to_string(type) + "-k" + std::to_string(k) + ".q";
            std::ofstream out_query(query_file);
            if (out_query.is_open()) {
                out_query << queries.rdbuf();
            } else {
                std::cerr << "Could not open file " << query_file << " for writing" << std::endl;
                exit(-1);
            }
        }
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
}
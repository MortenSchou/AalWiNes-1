//
// Created by Morten on 16-04-2020.
//

#include <aalwines/model/builders/TopologyZooBuilder.h>
#include <aalwines/model/Network.h>
#include <aalwines/synthesis/RouteConstruction.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace po = boost::program_options;
using namespace aalwines;

void make_query(Network& network, const size_t type, const size_t k, std::ostream& s, size_t n){
    auto size = (network.size() - 1) / n;
    switch (type) {
        default:
        case 1:
            // Type 1: Find any single step, single stack-size.
            s << "<.> . <.> " << k << " DUAL" << std::endl;
            break;
        case 2:
            // Type 2: Find a path from first to last router.
            s << "<.> ";
            s << "[.#" << network.get_router(0)->name() << "] .* [" << network.get_router(network.size()-1-(n==1?1:0))->name() << "#.]";
            s << " <.> " << k << " DUAL" << std::endl;
            break;
        case 3:
            // Type 3: Go through specific interfaces connecting each layer. (N=1 doesn't really work here...)
            s << "<.> ";
            for (size_t i = 0, r = 0; i < n - 1; ++i, r+=size) {
                s << ".* [" << network.get_router(r + (i%size))->name() << "#" << network.get_router(r + (i%size) + size + (i==0?1:0))->name() << "] ";
                if (i==0) r++;
            }
            s << ".* <.> " << k << " DUAL" << std::endl;
            break;
        case 4: {
            // Type 4: Is there a loop for any interface (OR over all interfaces)
            s << "<.> .* (";
            bool first = true;
            for (auto inf : network.all_interfaces()) {
                if (inf->source()->is_null() || inf->target()->is_null()) continue;
                if (!first) {
                    s << " | ";
                }
                s << "([" << inf->source()->name() << "#" << inf->target()->name() << "] .* ["
                  << inf->source()->name() << "#" << inf->target()->name() << "])";
                first = false;
            }
            s << ") .* <.> " << k << " DUAL" << std::endl;
            break;
        }
        case 5: {
            // Type 5: A detour for the 'middle' interface.
            auto inf = network.all_interfaces()[network.all_interfaces().size()/2];
            s << "<.> "
              << "[.#" << inf->source()->name() << "] [^" << inf->source()->name() << "#" << inf->target()->name()
              << "]+ [" << inf->target()->name() << "#.]"
              << " <.> " << k << " DUAL" << std::endl;
            break;
        }
    }
}

void make_query2(Network& network, const size_t type, const size_t k, const std::string& name, bool mpls) {
    std::string dot, dot_star, dot_dot_plus;
    if (mpls) {
        dot = "<smpls ip>";
        dot_star = "<(mpls* smpls)? ip>";
        dot_dot_plus = "<mpls+ smpls ip>";
    } else {
        dot = "<.>";
        dot_star = "<.*>";
        dot_dot_plus = "<.+ .>";
    }
    switch (type) {
        default:
        case 1: { // Transparency: <.> X .* Y <.* . .> k
            for(auto &r_x : network.get_all_routers()){
                if(r_x->is_null()) continue;
                for(auto &r_y : network.get_all_routers()){
                    if (r_y->is_null()) continue;
                    auto query_file = name + "k" + std::to_string(k) + "-transparency-" + r_x->name() + "-" + r_y->name() + ".q";
                    std::ofstream out_query(query_file);
                    if (out_query.is_open()) {
                        out_query << dot << " [.#" << r_x->name() << "] .* [" << r_y->name() << "#.] " << dot_dot_plus << " " << k << " DUAL" << std::endl;
                    } else {
                        std::cerr << "Could not open file " << query_file << " for writing" << std::endl;
                        exit(-1);
                    }
                }
            }
            break;
        }
        case 2: { // Waypointing: <.*> X [^Z]* Y <.*>
            for(auto &r_x : network.get_all_routers()){
                if(r_x->is_null()) continue;
                for(auto &r_y : network.get_all_routers()){
                    if (r_x == r_y || r_y->is_null()) continue;
                    for(auto &r_z : network.get_all_routers()) {
                        if (r_x == r_z || r_y == r_z || r_z->is_null()) continue;
                        auto query_file = name + "k" + std::to_string(k) + "-waypoint-" + r_x->name() + "-" + r_z->name() + "-" + r_y->name() + ".q";
                        std::ofstream out_query(query_file);
                        if (out_query.is_open()) {
                            out_query << dot_star << " [.#" << r_x->name() << "] [^.#" << r_z->name() << "]* [" << r_y->name() << "#.] " << dot_star << " " << k << " DUAL" << std::endl;
                        } else {
                            std::cerr << "Could not open file " << query_file << " for writing" << std::endl;
                            exit(-1);
                        }
                    }
                }
            }
            break;
        }
        case 3: { // Reachability: <.*> X .* Y <.*>
            for(auto &r_x : network.get_all_routers()){
                if(r_x->is_null()) continue;
                for(auto &r_y : network.get_all_routers()){
                    if (r_x == r_y || r_y->is_null()) continue;
                    auto query_file = name + "k" + std::to_string(k) + "-reachability-" + r_x->name() + "-" + r_y->name() + ".q";
                    std::ofstream out_query(query_file);
                    if (out_query.is_open()) {
                        out_query << dot_star << " [.#" << r_x->name() << "] .* [" << r_y->name() << "#.] " << dot_star << " " << k << " DUAL" << std::endl;
                    } else {
                        std::cerr << "Could not open file " << query_file << " for writing" << std::endl;
                        exit(-1);
                    }
                }
            }
            break;
        }
        case 4: { // <.*> X .* Y <.>
            for(auto &r_x : network.get_all_routers()){
                if(r_x->is_null()) continue;
                for(auto &r_y : network.get_all_routers()){
                    if (r_x == r_y || r_y->is_null()) continue;
                    auto query_file = name + "k" + std::to_string(k) + "-startosingle-" + r_x->name() + "-" + r_y->name() + ".q";
                    std::ofstream out_query(query_file);
                    if (out_query.is_open()) {
                        out_query << dot_star << " [.#" << r_x->name() << "] .* [" << r_y->name() << "#.] " << dot << " " << k << " DUAL" << std::endl;
                    } else {
                        std::cerr << "Could not open file " << query_file << " for writing" << std::endl;
                        exit(-1);
                    }
                }
            }
            break;
        }
        case 5: { // Loop: <.*> X .+ X <.*>
            for(auto &r_x : network.get_all_routers()) {
                if (r_x->is_null()) continue;
                auto query_file = name + "k" + std::to_string(k) + "-loop-" + r_x->name() + ".q";
                std::ofstream out_query(query_file);
                if (out_query.is_open()) {
                    out_query << dot_star << " [.#" << r_x->name() << "] .+ [" << r_x->name() << "#.] " << dot_star << " " << k << " DUAL" << std::endl;
                } else {
                    std::cerr << "Could not open file " << query_file << " for writing" << std::endl;
                    exit(-1);
                }
            }
            break;
        }
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
    size_t N = 1;
    size_t max_k = 3;
    bool mpls = false;
    bool dot_graph = false;
    bool print_simple = false;
    po::options_description generate("Test Options");
    generate.add_options()
            ("size,N", po::value<size_t>(&N), "the size variable (N)")
            ("max_k,k", po::value<size_t>(&max_k), "the maximal number of failures (k) for the queries generated")
            ("mpls,m", po::bool_switch(&mpls), "Use mpls smpls and ip instead of wilcard <.>")
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

    auto size = N; // Make it linear in N. // We tried quadratic, it was too much.

    auto network = make_large([&topo_zoo](){ return TopologyZooBuilder::parse(topo_zoo); }, size);

    // Construct routes on network
    uint64_t i = 42;
    auto next_label = [&i](){return Query::label_t(Query::type_t::MPLS, 0, i++);};
    auto cost = [](const Interface* interface){
        return interface->source()->coordinate() && interface->target()->coordinate() ?
               (interface->source()->coordinate().value() == interface->target()->coordinate().value() ? 100 : // 100 km between layers.
        interface->source()->coordinate()->distance_to(interface->target()->coordinate().value())) : 1000; }; // 1000 km if they don't have coordinates.
    // Make data flow for all pairs of routers.
    for(auto &r : network.get_all_routers()){
        if(r->is_null()) continue;
        for(auto &r_p : network.get_all_routers()){
            Interface *i1, *i2;
            if (r == r_p || r_p->is_null() ||
                (i1 = r->get_null_interface()) == nullptr || (i2 = r_p->get_null_interface()) == nullptr) continue;
            RouteConstruction::make_data_flow(i1, i2, next_label, cost);
        }
    }
    // Make reroute for all interfaces
    for(auto& inf : network.all_interfaces()){
        if (inf->target()->is_null() || inf->source()->is_null()) continue;
        RouteConstruction::make_reroute(inf, next_label, cost);
    }

    if (dot_graph) {
        network.print_dot_undirected(std::cout);
    }
    if (print_simple) {
        network.print_simple(std::cout);
    }

    for (size_t k = 0; k <= max_k; ++k) {
        for (size_t type = 1; type <= 5; ++type) {
            std::filesystem::create_directory(name + "-" + std::to_string(N));
            make_query2(network, type, k, name + "-" + std::to_string(N) + "/", mpls);
        }
    }

    auto topo_file = name + "-" + std::to_string(N) + "-topo.xml";
    std::ofstream out_topo(topo_file);
    if (out_topo.is_open()) {
        network.write_prex_topology(out_topo);
    } else {
        std::cerr << "Could not open file " << topo_file << " for writing" << std::endl;
        exit(-1);
    }
    auto routing_file = name + "-" + std::to_string(N) + "-routing.xml";
    std::ofstream out_route(routing_file);
    if (out_route.is_open()) {
        network.write_prex_routing(out_route);
    } else {
        std::cerr << "Could not open file " << routing_file << " for writing" << std::endl;
        exit(-1);
    }
}
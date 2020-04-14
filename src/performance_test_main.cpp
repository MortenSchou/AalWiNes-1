//
// Created by Dan Kristiansen on 4/9/20.
//

#include <aalwines/model/Network.h>
#include <aalwines/synthesis/FastRerouting.h>
#include <boost/program_options/options_description.hpp>
#include <fstream>
#include <random>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace aalwines;

void write_query(const Router* start_router, const Router* end_router, std::stringstream* s){
    *s << "<.> ";
    *s << "[.#" + start_router->name() + "]";
    *s << " .* ";
    *s << "[" + end_router->name() + "#.]";
    *s << " <.> 0 DUAL\n";
}

void write_query_through(const Router* start_router, const Router* through_router, const Router* end_router, std::stringstream* s){
    *s << "<.> ";
    *s << "[.#" + start_router->name() + "]";
    *s << " .* ";
    *s << "[.#" + through_router->name() + "]";
    *s << " .* ";
    *s << "[" + end_router->name() + "#.]";
    *s << " <.> 0 DUAL\n";
}

void create_path(Network& network, std::function<FastRerouting::label_t(void)>& next_label, uint64_t* i){
    /*Dynamic flow
    std::vector<std::vector<const Router*>> vector;
    vector.push_back({network.get_router(0), network.get_router(1),
                         network.get_router(3), network.get_router(4),
                         network.get_router(2), network.get_router(3)});
    vector.push_back({network.get_router(0), network.get_router(1),
                         network.get_router(3)});
    vector.push_back({network.get_router(0), network.get_router(2), network.get_router(3)});
    vector.push_back({network.get_router(0), network.get_router(2),
                         network.get_router(4), network.get_router(3)});
     * int randomDecision = rand()%3;
     * FastRerouting::make_data_flow(network.get_router(0)->find_interface("iRouter0"),
                                  network.get_router(3)->find_interface("iRouter3"),
                                  next_label, vector[randomDecision]);
                                  */
    //Static flow for testing
    FastRerouting::make_data_flow(network.get_router(0)->find_interface("iRouter0"),
                                  network.get_router(3)->find_interface("iRouter3"),
                                  next_label, {network.get_router(0), network.get_router(2), network.get_router(3)});
}

void create_flow(Network *pNetwork, std::function<FastRerouting::label_t(void)> function) {
    Interface* pre_router_inf = nullptr;
    for(auto& r : pNetwork->get_all_routers()){
        if(auto inf = r->get_null_interface()){
            if(pre_router_inf){
                FastRerouting::make_data_flow(pre_router_inf, inf, function);
            }
            pre_router_inf = inf;
        } else {
            pre_router_inf = nullptr;
        }
    }
}

Network construct_base(size_t concat, size_t inject, uint64_t *pInt, std::function<FastRerouting::label_t(void)> function) {
    auto network = Network::construct_synthetic_network();
    Router* start_router = network.get_router(0);
    Router* end_router = network.get_router(3);
    Interface* middle_interface = network.get_router(2)->find_interface("Router3");

    for(size_t i = 1; i < concat; i++){
        Network new_network = Network::construct_synthetic_network();
        end_router = new_network.get_router(3);
        int router_offset = i > 1 ? (i-1) * 5 + 1 : 0;
        std::vector<Interface*> links = {network.get_router(2 + router_offset)->find_interface("iRouter2"),
                                         network.get_router(4 + router_offset)->find_interface("iRouter4")};
        std::vector<Interface*> start_links = {new_network.get_router(0)->find_interface("iRouter0"),
                                               new_network.get_router(1)->find_interface("iRouter1")};
        if(i % 2 == 1){
            middle_interface = new_network.get_router(2)->find_interface("Router3");
        }
        network.concat_network(links, std::move(new_network), start_links);
    }
    create_flow(&network, function);

    for(size_t i = 1; i < inject; i++){
        Network injecting_network = construct_base(concat, inject - 1, pInt, function);
        std::vector<Interface*> links = {middle_interface};
        std::vector<Network::data_flow> flows = {Network::data_flow(start_router->get_null_interface(), end_router->get_null_interface(),
                                                           {Query::MPLS, 0, 4},{Query::MPLS, 0, 4})};
        network.inject_network(links, std::move(injecting_network), flows);
    }
    return network;
}

int main(int argc, const char** argv) {
    po::options_description opts;
    opts.add_options()
            ("help,h", "produce help message");

    size_t size = 1;
    size_t concat_inject = 0;
    size_t number_networks = 1;
    size_t number_dataflow = 0;
    size_t base_concat = 1;
    size_t base_inject = 1;
    size_t base_repeat = 1;
    po::options_description generate("Test Options");
    generate.add_options()
            ("size,s", po::value<size_t>(&size), "size of synthetic network")
            ("mode,m", po::value<size_t>(&concat_inject), "0 = concat and 1 = inject")
            ("networks,n", po::value<size_t>(&number_networks), "amount of test networks")
            ("flow,f", po::value<size_t>(&number_dataflow), "amount of data flow in the network")
            ("inject,i", po::value<size_t>(&base_inject), "amount of injected networks in base")
            ("concat,c", po::value<size_t>(&base_concat), "amount of concatenated networks in base")
            ("reputation,r", po::value<size_t>(&base_repeat), "amount of base network reputations")
            ;
    opts.add(generate);
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);

    std::string name =
            "Network_C" + std::to_string(base_concat) + "_I" + std::to_string(base_inject) + "_R" + std::to_string(base_repeat);
    uint64_t i = 42;
    std::function<FastRerouting::label_t(void)> next_label = [&i]() {
        return Query::label_t(Query::type_t::MPLS, 0, i++);
    };

    Network synthetic_network = construct_base(base_concat, base_inject, &i, next_label);

    synthetic_network.print_dot_undirected(std::cout);

    //create_flow(&synthetic_network, next_label);

    std::stringstream queries;
    //TODO write query

    std::ofstream out_topo(name + "-topo.xml");
    if (out_topo.is_open()) {
        synthetic_network.write_prex_topology(out_topo);
    } else {
        std::cerr << "Could not open --write-topology file for writing" << std::endl;
        exit(-1);
    }
    std::ofstream out_route(name + "-routing.xml");
    if (out_route.is_open()) {
        synthetic_network.write_prex_routing(out_route);
    } else {
        std::cerr << "Could not open --write-routing file for writing" << std::endl;
        exit(-1);
    }
    std::ofstream out_query(name + "-queries.q");
    if (out_query.is_open()) {
        out_query << queries.rdbuf();
    } else {
        std::cerr << "Could not open --write-query file for writing" << std::endl;
        exit(-1);
    }

    std::ofstream out_stat(name + "-stat.txt");
    if (out_stat.is_open()) {
        synthetic_network.write_stats(out_stat);
    } else {
        std::cerr << "Could not open --write-stats file for writing" << std::endl;
        exit(-1);
    }
}
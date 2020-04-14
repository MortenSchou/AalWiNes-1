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

void write_query_through(const Router* start_router, const Interface* through_interface, const Router* end_router, const size_t failover, std::stringstream* s){
    *s << "<.> ";
    *s << "[.#" + start_router->name() + "]";
    *s << " .* ";
    *s << "[" + through_interface->source()->name() + "#" + through_interface->target()->name() + "]";
    *s << " .* ";
    *s << "[" + end_router->name() + "#.]";
    *s << " <.> " << failover << " DUAL\n";
}

void create_flow(Network &pNetwork, std::function<FastRerouting::label_t(void)> function) {
    Interface* pre_router_inf = nullptr;
    for(auto& r : pNetwork.get_all_routers()){
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

struct synt_net {
    Network network;
    std::pair<Interface*, RoutingTable::label_t> inject_flow_start;
    std::pair<Interface*, RoutingTable::label_t> inject_flow_end;
    Interface* mid;
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>> concat_flow_starts;
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>> concat_flow_ends;
};

synt_net inject_synt(synt_net&& net1, synt_net&& net2) {
    std::vector<Interface*> links = {net1.mid};
    std::vector<Network::data_flow> flows = {Network::data_flow{net2.inject_flow_start, net2.inject_flow_end}};
    net1.network.inject_network(links, std::move(net2.network), flows);
    return synt_net{std::move(net1.network), net1.inject_flow_start, net1.inject_flow_end, net2.mid, net1.concat_flow_starts, net1.concat_flow_ends};
}
synt_net concat_synt(std::vector<synt_net>&& nets) {
    assert(nets.size() % 2 == 1);
    auto&& net = nets[0];
    auto& flow_ends = net.concat_flow_ends;
    for (size_t i = 1; i < nets.size(); ++i) {
        net.network.concat_network(flow_ends, std::move(nets[i].network), nets[i].concat_flow_starts);
        flow_ends = nets[i].concat_flow_ends;
    }
    return synt_net{std::move(net.network), net.inject_flow_start, nets[nets.size()-1].inject_flow_end,
                    nets[(nets.size()-1)/2].mid, net.concat_flow_starts, flow_ends};
}

synt_net make_base(
        const std::function<FastRerouting::label_t(void)>& next_label,
        const std::function<FastRerouting::label_t(void)>& cur_label,
        const std::function<FastRerouting::label_t(void)>& peek_label) {
    auto network = Network::construct_synthetic_network();
    std::pair<Interface*, RoutingTable::label_t> inject_flow_start;
    std::pair<Interface*, RoutingTable::label_t> inject_flow_end;
    auto middle_interface = network.get_router(2)->find_interface("Router3");

    // Concat-flows/pairs: 0 -> 2, 1 -> 4
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>> concat_flow_starts;
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>> concat_flow_ends;
    concat_flow_starts.emplace_back(network.get_router(0)->get_null_interface(), std::vector<RoutingTable::label_t>{});
    concat_flow_starts.emplace_back(network.get_router(1)->get_null_interface(), std::vector<RoutingTable::label_t>{});
    concat_flow_ends.emplace_back(network.get_router(2)->get_null_interface(), std::vector<RoutingTable::label_t>{});
    concat_flow_ends.emplace_back(network.get_router(4)->get_null_interface(), std::vector<RoutingTable::label_t>{});

    // Make data flow between all pairs of routers.
    for(auto& r1 : network.get_all_routers()){
        if (r1->is_null()) continue;
        auto interface1 = r1->get_null_interface();
        if (interface1 == nullptr) continue;
        for(auto& r2 : network.get_all_routers()){
            if (r1 == r2 || r2->is_null()) continue;
            auto interface2 = r2->get_null_interface();
            if (interface2 == nullptr) continue;
            auto pre_label = peek_label();
            FastRerouting::make_data_flow(interface1, interface2, next_label);
            auto post_label = cur_label();
            // Store pre and post labels for flows:
            // Inject-flow: 0 -> 4
            if (r1.get() == network.get_router(0) && r2.get() == network.get_router(4)) {
                inject_flow_start = std::make_pair(interface1, pre_label);
                inject_flow_end = std::make_pair(interface2, post_label);
            }
            for (auto& [inf, labels] : concat_flow_starts) {
                if (inf == interface1) {
                    labels.push_back(pre_label);
                }
            }
            for (auto& [inf, labels] : concat_flow_ends) {
                if (inf == interface2) {
                    labels.insert(labels.begin(), post_label);
                }
            }
        }
    }
    // Make reroute for links: 1 -> 3 and 2 -> 4.
    FastRerouting::make_reroute(network.get_router(1)->find_interface("Router3"), next_label);
    FastRerouting::make_reroute(network.get_router(2)->find_interface("Router4"), next_label);

    return synt_net{std::move(network), inject_flow_start, inject_flow_end,
                    middle_interface, concat_flow_starts, concat_flow_ends};
}

synt_net construct_base(size_t concat, size_t inject,
                        const std::function<FastRerouting::label_t(void)>& next_label,
                        const std::function<FastRerouting::label_t(void)>& cur_label,
                        const std::function<FastRerouting::label_t(void)>& peek_label) {
    std::vector<synt_net> nets;
    for (size_t i = 0; i < concat; ++i) {
        nets.push_back(make_base(next_label, cur_label, peek_label));
    }
    auto net = concat_synt(std::move(nets));
    for (size_t j = 1; j < inject; j++) {
        std::vector<synt_net> nets2;
        for (size_t i = 0; i < concat; ++i) {
            nets2.push_back(make_base(next_label, cur_label, peek_label));
        }
        net = inject_synt(std::move(net), concat_synt(std::move(nets2)));
    }
    return net;
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
    size_t failover_query = 0;
    po::options_description generate("Test Options");
    generate.add_options()
            ("size,s", po::value<size_t>(&size), "size of synthetic network")
            ("mode,m", po::value<size_t>(&concat_inject), "0 = concat and 1 = inject")
            ("networks,n", po::value<size_t>(&number_networks), "amount of test networks")
            ("flow,f", po::value<size_t>(&number_dataflow), "amount of data flow in the network")
            ("inject,i", po::value<size_t>(&base_inject), "amount of injected networks in base")
            ("concat,c", po::value<size_t>(&base_concat), "amount of concatenated networks in base")
            ("reputation,r", po::value<size_t>(&base_repeat), "amount of base network reputations")
            ("failover,k", po::value<size_t>(&failover_query), "failover paths")
            ;
    opts.add(generate);
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);

    std::string name =
            "Network_C" + std::to_string(base_concat) + "_I" + std::to_string(base_inject) + "_R" + std::to_string(base_repeat) + "_K" + std::to_string(failover_query);
    uint64_t i = 42;
    auto next_label = [&i]() {
        i++;
        return Query::label_t(Query::type_t::MPLS, 0, i);
    };
    auto cur_label = [&i]() {
        return Query::label_t(Query::type_t::MPLS, 0, i);
    };
    auto peek_label = [&i]() {
        return Query::label_t(Query::type_t::MPLS, 0, i+1);
    };

    auto result = construct_base(3, 2, next_label, cur_label, peek_label);
    auto synthetic_network = std::move(result.network);

    synthetic_network.print_dot_undirected(std::cout);

    std::stringstream queries;
    //Query from start interface -> mid interface -> end interface
    write_query_through(result.inject_flow_start.first->source(), result.mid, result.inject_flow_end.first->source(), failover_query, &queries);

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
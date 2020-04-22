//
// Created by Dan Kristiansen on 4/9/20.
//

#include <aalwines/model/Network.h>
#include <aalwines/synthesis/RouteConstruction.h>
#include <boost/program_options/options_description.hpp>
#include <fstream>
#include <random>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace aalwines;

void write_query_through(const Router* start_router, const Interface* through_interface, const Router* end_router, const size_t failover, std::ostream& s){
    s << "<.> ";
    s << "[.#" + start_router->name() + "]";
    s << " .* ";
    s << "[" + through_interface->source()->name() + "#" + through_interface->target()->name() + "]";
    s << " .* ";
    s << "[" + end_router->name() + "#.]";
    s << " <.> " << failover << " DUAL\n";
}

struct SyntheticNetwork {
    SyntheticNetwork(Network&& network,
    std::pair<Interface*, RoutingTable::label_t>&& inject_flow_start,
    std::pair<Interface*, RoutingTable::label_t>&& inject_flow_end,
    Interface* mid,
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>>&& concat_flow_starts,
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>>&& concat_flow_ends) :
    network(std::move(network)), inject_flow_start(std::move(inject_flow_start)), inject_flow_end(std::move(inject_flow_end)),
    mid(mid), concat_flow_starts(std::move(concat_flow_starts)), concat_flow_ends(std::move(concat_flow_ends)){
        via.push_back(mid);
    };
    Network network;
    std::pair<Interface*, RoutingTable::label_t> inject_flow_start;
    std::pair<Interface*, RoutingTable::label_t> inject_flow_end;
    Interface* mid;
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>> concat_flow_starts;
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>> concat_flow_ends;
    std::vector<Interface*> via;

    void make_query_through_mid(size_t fails, std::ostream& s){
        write_query_through(inject_flow_start.first->source(), mid, inject_flow_end.first->source(), fails, s);
    }
    void make_query_through_via_interfaces(size_t fails, std::ostream& s){
        s << "<.> ";
        s << "[.#" + inject_flow_start.first->source()->name() + "]";
        s << " .* ";
        for (auto via_interface : via) {
            s << "[" + via_interface->source()->name() + "#" + via_interface->target()->name() + "]";
            s << " .* ";
        }
        s << "[" + inject_flow_end.first->source()->name() + "#.]";
        s << " <.> " << fails << " DUAL\n";
    }
    void inject(SyntheticNetwork&& other) {
        std::vector<Interface *> links = {mid};
        std::vector<Network::data_flow> flows = {Network::data_flow{other.inject_flow_start, other.inject_flow_end}};
        network.inject_network(links, std::move(other.network), flows);
        via.insert(via.erase(std::find(via.begin(), via.end(), mid)), other.via.begin(), other.via.end());
        mid = other.mid;
    }
    static SyntheticNetwork concat(std::vector<SyntheticNetwork>&& nets) {
        assert(nets.size() % 2 == 1);
        auto&& net = nets[0];
        for (size_t i = 1; i < nets.size(); ++i) {
            net.concat(std::move(nets[i]));
        }
        net.mid = nets[(nets.size()-1)/2].mid;
        net.inject_flow_end = nets[nets.size() - 1].inject_flow_end;
        return std::move(net);
    }

private:
    void concat(SyntheticNetwork&& other) {
        network.concat_network(concat_flow_ends, std::move(other.network), other.concat_flow_starts);
        concat_flow_ends = other.concat_flow_ends;
        via.insert(via.end(), other.via.begin(), other.via.end());
    }
};
struct label_generator {
    explicit label_generator(uint64_t start) : i(start) {};
    auto next() {
        i++;
        return Query::label_t(Query::type_t::MPLS, 0, i);
    };
    auto next_fn() {
        return [this](){ return next(); };
    }
    auto current() {
        return Query::label_t(Query::type_t::MPLS, 0, i);
    };
    auto peek() {
        return Query::label_t(Query::type_t::MPLS, 0, i+1);
    };
private:
    uint64_t i;
};

SyntheticNetwork make_base(label_generator& labels) {
    auto network = Network::construct_synthetic_network();
    std::pair<Interface*, RoutingTable::label_t> inject_flow_start;
    std::pair<Interface*, RoutingTable::label_t> inject_flow_end;
    auto middle_interface = network.get_router(2)->find_interface("Router3");

    // Concat-flows/pairs: 0 -> 2, 1 -> 4
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>> concat_flow_starts;
    std::vector<std::pair<Interface*, std::vector<RoutingTable::label_t>>> concat_flow_ends;
    concat_flow_starts.emplace_back(network.get_router(0)->get_null_interface(), std::vector<RoutingTable::label_t>{});
    concat_flow_starts.emplace_back(network.get_router(1)->get_null_interface(), std::vector<RoutingTable::label_t>{});
    concat_flow_ends.emplace_back(network.get_router(4)->get_null_interface(), std::vector<RoutingTable::label_t>{});
    concat_flow_ends.emplace_back(network.get_router(3)->get_null_interface(), std::vector<RoutingTable::label_t>{});

    // Make data flow between all pairs of routers.
    for(auto& r1 : network.get_all_routers()){
        if (r1->is_null()) continue;
        auto interface1 = r1->get_null_interface();
        if (interface1 == nullptr) continue;
        for(auto& r2 : network.get_all_routers()){
            if (r1 == r2 || r2->is_null()) continue;
            auto interface2 = r2->get_null_interface();
            if (interface2 == nullptr) continue;
            auto pre_label = labels.peek();
            RouteConstruction::make_data_flow(interface1, interface2, labels.next_fn());
            auto post_label = labels.current();
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
    RouteConstruction::make_reroute(network.get_router(1)->find_interface("Router3"), labels.next_fn());
    RouteConstruction::make_reroute(network.get_router(2)->find_interface("Router4"), labels.next_fn());

    return SyntheticNetwork(std::move(network), std::move(inject_flow_start), std::move(inject_flow_end),
                            middle_interface, std::move(concat_flow_starts), std::move(concat_flow_ends));
}

extern SyntheticNetwork construct_base(size_t concat, size_t inject, size_t repetitions, label_generator& labels,
                                       const std::function<SyntheticNetwork(size_t, size_t, size_t, label_generator&)>& make_inner);

SyntheticNetwork construct_nesting(size_t concat, size_t inject, size_t repetitions, label_generator& labels) {
    if (repetitions == 0) {
        return make_base(labels);
    } else {
        return construct_base(concat, inject, repetitions - 1, labels, construct_nesting);
    }
}

SyntheticNetwork construct_base(size_t concat, size_t inject, size_t repetitions, label_generator& labels,
                                const std::function<SyntheticNetwork(size_t, size_t, size_t, label_generator&)>& make_inner) {
    std::vector<SyntheticNetwork> nets;
    for (size_t i = 0; i < concat; ++i) {
        nets.push_back(make_inner(concat, inject, repetitions, labels));
    }
    auto net = SyntheticNetwork::concat(std::move(nets));
    for (size_t j = 1; j < inject; j++) {
        std::vector<SyntheticNetwork> nets2;
        for (size_t i = 0; i < concat; ++i) {
            nets2.push_back(make_inner(concat, inject, repetitions, labels));
        }
        net.inject(SyntheticNetwork::concat(std::move(nets2)));
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
    bool dot_graph = false;
    bool print_simple = false;
    po::options_description generate("Test Options");
    generate.add_options()
            ("size,s", po::value<size_t>(&size), "size of synthetic network")
            ("mode,m", po::value<size_t>(&concat_inject), "0 = concat and 1 = inject")
            ("networks,n", po::value<size_t>(&number_networks), "amount of test networks")
            ("flow,f", po::value<size_t>(&number_dataflow), "amount of data flow in the network")
            ("inject,i", po::value<size_t>(&base_inject), "amount of injected networks in base")
            ("concat,c", po::value<size_t>(&base_concat), "amount of concatenated networks in base")
            ("repetition,r", po::value<size_t>(&base_repeat), "amount of base network repetition")
            ("failover,k", po::value<size_t>(&failover_query), "failover paths")
            ("dot,d", po::bool_switch(&dot_graph), "print dot graph output")
            ("print_simple,p", po::bool_switch(&print_simple), "print simple routing output")
            ;
    opts.add(generate);
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);

    std::string name =
            "Network_C" + std::to_string(base_concat) + "_I" + std::to_string(base_inject) + "_R" + std::to_string(base_repeat) + "_K" + std::to_string(failover_query);

    label_generator labels(42);
    auto result = construct_nesting(base_concat, base_inject, base_repeat, labels);

    if (dot_graph) {
        result.network.print_dot_undirected(std::cout);
    }
    if (print_simple) {
        result.network.print_simple(std::cout);
    }

    std::stringstream queries;
    //Query from start interface -> mid interface -> end interface
    result.make_query_through_via_interfaces(failover_query, queries);

    std::ofstream out_topo(name + "-topo.xml");
    if (out_topo.is_open()) {
        result.network.write_prex_topology(out_topo);
    } else {
        std::cerr << "Could not open --write-topology file for writing" << std::endl;
        exit(-1);
    }
    std::ofstream out_route(name + "-routing.xml");
    if (out_route.is_open()) {
        result.network.write_prex_routing(out_route);
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
        result.network.write_stats(out_stat);
    } else {
        std::cerr << "Could not open --write-stats file for writing" << std::endl;
        exit(-1);
    }
}
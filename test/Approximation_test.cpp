//
// Created by Morten on 21-04-2020.
//

#define BOOST_TEST_MODULE Approximation

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>
#include <aalwines/synthesis/FastRerouting.h>
#include <fstream>
#include <iostream>

using namespace aalwines;

BOOST_AUTO_TEST_CASE(OverApproximationTest) {
    std::vector<std::string> names{"Router1", "Router2", "Router3", "Router4", "Router5", "Router6"};
    std::vector<std::vector<std::string>> links{{"Router2", "Router4"},
                                                {"Router1",  "Router3", "Router5"},
                                                {"Router2",  "Router6"},
                                                {"Router1",  "Router5"},
                                                {"Router2",  "Router4", "Router6"},
                                                {"Router5",  "Router3"}};
    auto network = Network::make_network(names, links);

    uint64_t i = 42;
    auto next_label = [&i](){return Query::label_t(Query::type_t::MPLS, 0, i++);};
    auto success = FastRerouting::make_data_flow(
            network.get_router(0)->get_null_interface(),
            network.get_router(2)->get_null_interface(), next_label);
    BOOST_CHECK_EQUAL(success, true);

    success = FastRerouting::make_reroute(network.get_router(0)->find_interface(names[1]), next_label);
    BOOST_CHECK_EQUAL(success, true);
    success = FastRerouting::make_reroute(network.get_router(1)->find_interface(names[2]), next_label);
    BOOST_CHECK_EQUAL(success, true);

    network.print_simple(std::cout);

    // TODO: Use verification here instead of printing to files.
    std::string name = "over-approx-example";
    auto topo_file = name + "-topo.xml";
    std::ofstream out_topo(topo_file);
    BOOST_CHECK_EQUAL(out_topo.is_open(), true);
    network.write_prex_topology(out_topo);

    auto routing_file = name + "-routing.xml";
    std::ofstream out_route(routing_file);
    BOOST_CHECK_EQUAL(out_route.is_open(), true);
    network.write_prex_routing(out_route);

    auto query_file = name + "-query.q";
    std::ofstream out_query(query_file);
    BOOST_CHECK_EQUAL(out_query.is_open(), true);
    out_query << "<[42]> [.#Router1] .* [.#Router4] .* [.#Router6] .* [Router3#.] <[45]> 0 DUAL" << std::endl;
    out_query << "<[42]> [.#Router1] .* [.#Router4] .* [.#Router6] .* [Router3#.] <[45]> 1 DUAL" << std::endl;
    out_query << "<[42]> [.#Router1] .* [.#Router4] .* [.#Router6] .* [Router3#.] <[45]> 2 DUAL" << std::endl;
}
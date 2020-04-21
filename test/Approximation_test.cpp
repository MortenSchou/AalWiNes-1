//
// Created by Morten on 21-04-2020.
//

#define BOOST_TEST_MODULE Approximation

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>
#include <aalwines/synthesis/FastRerouting.h>
#include <fstream>
#include <iostream>
#include <pdaaal/SolverAdapter.h>
#include <aalwines/model/NetworkPDAFactory.h>
#include <aalwines/query/QueryBuilder.h>
#include <pdaaal/Reducer.h>
#include <aalwines/utils/outcome.h>


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

    Builder builder(network);
    {
        std::string query(   "<[42]> [.#Router1] .* [.#Router4] .* [.#Router6] .* [Router3#.] <[45]> 0 DUAL\n"
                                "<[42]> [.#Router1] .* [.#Router4] .* [.#Router6] .* [Router3#.] <[45]> 2 DUAL\n"
                                "<[42]> [.#Router1] .* [.#Router4] .* [.#Router6] .* [Router3#.] <[45]> 2 DUAL\n");

        std::istringstream out_query(query);
        builder.do_parse(out_query);
        int query_no = 0;
        pdaaal::SolverAdapter solver;
        std::stringstream proof;

        for(auto& q : builder._result) {
            std::cout << "\nQ" << query_no++ << std::endl;
            utils::outcome_t result = utils::MAYBE;
            std::vector<Query::mode_t> modes{q.approximation()};
            bool was_dual = q.approximation() == Query::DUAL;
            if (was_dual)
                modes = std::vector<Query::mode_t>{Query::OVER, Query::UNDER};

            std::pair<size_t, size_t> reduction;
            std::vector<pdaaal::TypedPDA<Query::label_t>::tracestate_t> trace;

            for (auto m : modes) {
                q.set_approximation(m);
                NetworkPDAFactory factory(q, network, false);
                auto pda = factory.compile();
                reduction = pdaaal::Reducer::reduce(pda, 0, pda.initial(), pda.terminal());

                auto solver_result1 = solver.post_star<pdaaal::Trace_Type::Any>(pda);
                bool engine_outcome = solver_result1.first;

                if (engine_outcome) {
                    trace = solver.get_trace(pda, std::move(solver_result1.second));
                    if (factory.write_json_trace(std::cout, trace))
                        result = utils::YES;
                }
                if (q.number_of_failures() == 0)
                    result = engine_outcome ? utils::YES : utils::NO;

                if (result == utils::MAYBE && m == Query::OVER && !engine_outcome)
                    result = utils::NO;
                if (result != utils::MAYBE)
                    break;
            }
            switch (result) {
                case utils::MAYBE:
                    std::cout << "null";
                    break;
                case utils::NO:
                    std::cout << "false";
                    break;
                case utils::YES:
                    std::cout << "true";
                    break;
            }
            std::cout << proof.str();
        }
    }
}
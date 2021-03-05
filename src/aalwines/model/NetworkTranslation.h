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
 * File:   NetworkTranslation.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 11-02-2021.
 */

#ifndef AALWINES_NETWORKTRANSLATION_H
#define AALWINES_NETWORKTRANSLATION_H

#include <aalwines/model/Query.h>
#include <aalwines/model/Network.h>
#include <pdaaal/ptrie_interface.h>

#include <utility>

namespace aalwines {

    template<Query::mode_t mode>
    struct State;

    template<>
    struct State<Query::mode_t::OVER> {
        using nfa_state_t = typename pdaaal::NFA<Query::label_t>::state_t;
        size_t _eid = 0; // which entry we are going for
        size_t _rid = 0; // which rule in that entry
        size_t _opid = std::numeric_limits<size_t>::max(); // which operation is the first in the rule (max = no operations left).
        const Interface* _inf = nullptr;
        const nfa_state_t* _nfa_state = nullptr;

        State() = default;

        State(size_t eid, size_t rid, size_t opid, const Interface* inf, const nfa_state_t* nfa_state)
        : _eid(eid), _rid(rid), _opid(opid), _inf(inf), _nfa_state(nfa_state) {};

        State(const Interface* inf, const nfa_state_t* nfa_state)
        : _inf(inf), _nfa_state(nfa_state) {};

        bool operator==(const State<Query::mode_t::OVER>& other) const {
            return _eid == other._eid && _rid == other._rid && _opid == other._opid
                   && _inf == other._inf && _nfa_state == other._nfa_state;
        }

        bool operator!=(const State<Query::mode_t::OVER>& other) const {
            return !(*this == other);
        }

        [[nodiscard]] size_t opid() const { return _opid; }
        [[nodiscard]] const Interface* interface() const { return _inf; }
        [[nodiscard]] const nfa_state_t* nfa_state() const { return _nfa_state; }
        [[nodiscard]] const RoutingTable::entry_t& entry() const {
            assert(!ops_done());
            return _inf->table()->entries()[_eid];
        }
        [[nodiscard]] const RoutingTable::forward_t& forward() const {
            assert(!ops_done());
            return _inf->table()->entries()[_eid]._rules[_rid];
        }
        [[nodiscard]] const RoutingTable::forward_t& forward(const RoutingTable::entry_t& entry) const {
            assert(!ops_done());
            return entry._rules[_rid];
        }

        [[nodiscard]] bool ops_done() const {
            return _opid == std::numeric_limits<size_t>::max();
        }

        [[nodiscard]] bool accepting() const {
            return ops_done() && (_inf == nullptr || !_inf->is_virtual()) && _nfa_state->_accepting;
        }

        static State perform_op(const State& state) {
            assert(!state.ops_done());
            return perform_op(state, state._inf->table()->entries()[state._eid]._rules[state._rid]);
        }

        static State perform_op(const State& state, const RoutingTable::forward_t& forward) {
            assert(!state.ops_done());
            if (state._opid + 2 == forward._ops.size()) {
                return State<Query::mode_t::OVER>(forward._via->match(), state._nfa_state);
            } else {
                return State<Query::mode_t::OVER>(state._eid, state._rid, state._opid + 1, state._inf,
                                                  state._nfa_state);
            }
        }

        static bool is_okay(const RoutingTable::entry_t& entry, const RoutingTable::forward_t& forward, size_t max_failures) {
            std::vector<const Interface*> new_failed;
            for (const auto& other : entry._rules) {
                if (other._priority < forward._priority) {
                    new_failed.push_back(
                            other._via); // Rules with higher priority (smaller numeric value) must fail for this forwarding rule to be applicable.
                }
            }
            if (new_failed.empty()) return true;
            std::sort(new_failed.begin(), new_failed.end());
            new_failed.erase(std::unique(new_failed.begin(), new_failed.end()), new_failed.end());
            return new_failed.size() <= max_failures;
        }
    } __attribute__((packed)); // packed is needed to make this work fast with ptries


    struct EdgeStatus {
        std::vector<const Interface*> _failed;
        std::vector<const Interface*> _used;

        // TODO: Optimization, once _failed.size() == max_failures, we no longer need to keep track of _used.
        EdgeStatus() = default;

        EdgeStatus(std::vector<const Interface*> failed, std::vector<const Interface*> used)
                : _failed(std::move(failed)), _used(std::move(used)) {};

        bool operator==(const EdgeStatus& other) const {
            return _failed == other._failed && _used == other._used;
        }

        bool operator!=(const EdgeStatus& other) const {
            return !(*this == other);
        }

        [[nodiscard]] std::optional<EdgeStatus>
        next_edge_state(const RoutingTable::entry_t& entry, const RoutingTable::forward_t& forward, size_t max_failures) const {
            assert(std::find(entry._rules.begin(), entry._rules.end(), forward) != entry._rules.end());
            auto lbf = std::lower_bound(_failed.begin(), _failed.end(), forward._via);
            if (lbf != _failed.end() && *lbf == forward._via)
                return std::nullopt; // Cannot use edge that was already assumed to by failed

            std::vector<const Interface*> new_failed;
            for (const auto& other : entry._rules) {
                if (other._priority < forward._priority) {
                    new_failed.push_back(
                            other._via); // Rules with higher priority (smaller numeric value) must fail for this forwarding rule to be applicable.
                }
            }
            if (new_failed.empty()) { // Speed up special (but common) case.
                return EdgeStatus(_failed, add_to_set(_used, forward._via));
            }
            std::sort(new_failed.begin(), new_failed.end());
            new_failed.erase(std::unique(new_failed.begin(), new_failed.end()), new_failed.end());

            std::vector<const Interface*> next_failed;
            std::set_union(new_failed.begin(), new_failed.end(), _failed.begin(), _failed.end(),
                           std::back_inserter(next_failed));
            if (new_failed.size() > max_failures) return std::nullopt;

            auto next_used = add_to_set(_used, forward._via);
            std::vector<const Interface*> intersection;
            std::set_intersection(new_failed.begin(), new_failed.end(), next_used.begin(), next_used.end(),
                                  std::back_inserter(intersection));
            if (!intersection.empty()) return std::nullopt; // Failed and used must be disjoint.

            return EdgeStatus(std::move(next_failed), std::move(next_used));
        }

        [[nodiscard]] bool soundness_check(size_t max_failures) const {
            // This checks the soundness of this structure. Can e.g. be used in assert statements in debug mode.
            if (!std::is_sorted(_failed.begin(), _failed.end()))
                return false; // Check that data structure is still sorted vector...
            if (!std::is_sorted(_used.begin(), _used.end())) return false;
            if (std::adjacent_find(_failed.begin(), _failed.end()) != _failed.end())
                return false; //... with unique elements. I.e. a set.
            if (std::adjacent_find(_used.begin(), _used.end()) != _used.end()) return false;
            if (_failed.size() > max_failures) return false; // Check |F| <= k
            std::vector<const Interface*> intersection;
            std::set_intersection(_failed.begin(), _failed.end(), _used.begin(), _used.end(),
                                  std::back_inserter(intersection));
            if (!intersection.empty()) return false; // Check F \cap U == \emptyset
            return true;
        }

    private:
        static std::vector<const Interface*> add_to_set(const std::vector<const Interface*>& set, const Interface* elem) {
            auto lb = std::lower_bound(set.begin(), set.end(), elem);
            if (lb != set.end() && *lb == elem) { // Elem is already in the set.
                return set;
            }
            std::vector<const Interface*> next_set;
            next_set.reserve(set.size() + 1);
            next_set.insert(next_set.end(), set.begin(), lb);
            next_set.push_back(elem);
            next_set.insert(next_set.end(), lb, set.end());
            return next_set;
        }
    };

    template<>
    struct State<Query::mode_t::EXACT> {
        using nfa_state_t = typename pdaaal::NFA<Query::label_t>::state_t;
        State<Query::mode_t::OVER> _s;
        EdgeStatus _edge_state;

        State() = default;
        State(size_t eid, size_t rid, size_t opid, const Interface* inf, const nfa_state_t* nfa_state, EdgeStatus edge_state)
        : _s(eid, rid, opid, inf, nfa_state), _edge_state(std::move(edge_state)) {};
        State(const Interface* inf, const nfa_state_t* nfa_state, EdgeStatus edge_state = EdgeStatus())
        : _s(inf, nfa_state), _edge_state(std::move(edge_state)) {};
        State(State<Query::mode_t::OVER>&& s, EdgeStatus edge_state)
        : _s(s), _edge_state(std::move(edge_state)) {};

        bool operator==(const State<Query::mode_t::EXACT>& other) const {
            return _s == other._s && _edge_state == other._edge_state;
        }
        bool operator!=(const State<Query::mode_t::EXACT>& other) const { return !(*this == other); }

        // Delegate access functions inner state structure.
        [[nodiscard]] size_t opid() const { return _s.opid(); }
        [[nodiscard]] const Interface* interface() const { return _s.interface(); }
        [[nodiscard]] const nfa_state_t* nfa_state() const { return _s.nfa_state(); }
        [[nodiscard]] const RoutingTable::entry_t& entry() const { return _s.entry(); }
        [[nodiscard]] const RoutingTable::forward_t& forward() const { return _s.forward(); }
        [[nodiscard]] const RoutingTable::forward_t& forward(const RoutingTable::entry_t& entry) const { return _s.forward(entry); }

        [[nodiscard]] bool ops_done() const { return _s.ops_done(); }
        [[nodiscard]] bool accepting() const { return _s.accepting(); }

        static State<Query::mode_t::EXACT> perform_op(const State<Query::mode_t::EXACT>& state) {
            return State<Query::mode_t::EXACT>(State<Query::mode_t::OVER>::perform_op(state._s), state._edge_state);
        }
        static State<Query::mode_t::EXACT> perform_op(const State<Query::mode_t::EXACT>& state, const RoutingTable::forward_t& forward) {
            return State<Query::mode_t::EXACT>(State<Query::mode_t::OVER>::perform_op(state._s, forward), state._edge_state);
        }

        [[nodiscard]] std::optional<EdgeStatus>
        next_edge_state(const RoutingTable::entry_t& entry, const RoutingTable::forward_t& forward, size_t max_failures) const {
            if (forward._via->is_virtual()) return _edge_state;
            return _edge_state.next_edge_state(entry, forward, max_failures);
        }
    };
}

// We need some boilerplate to make this work with ptries (i.e. converting to a byte-vector).
// This could be done automatically if we used tuples instead of structs, but that gets quite annoying.
// It would be useful to be able to iterate through the member fields of a struct, but I don't know if that is possible.
namespace pdaaal::utils {
    template<>
    struct byte_vector_converter<aalwines::EdgeStatus> {
        using T = aalwines::EdgeStatus;

        static constexpr size_t size(const T& data) {
            return byte_vector_converter<std::vector<const aalwines::Interface*>>::size(data._failed) +
                   byte_vector_converter<std::vector<const aalwines::Interface*>>::size(data._used);
        }
        static constexpr void push_back_bytes(std::vector<std::byte>& result, const T& data) {
            byte_vector_converter<std::vector<const aalwines::Interface*>>::push_back_bytes(result, data._failed);
            byte_vector_converter<std::vector<const aalwines::Interface*>>::push_back_bytes(result, data._used);
        }
        static constexpr void from_bytes(const std::vector<std::byte>& bytes, size_t& bytes_id, T& data) {
            byte_vector_converter<std::vector<const aalwines::Interface*>>::from_bytes(bytes, bytes_id, data._failed);
            byte_vector_converter<std::vector<const aalwines::Interface*>>::from_bytes(bytes, bytes_id, data._used);
        }
    };
    template<>
    struct byte_vector_converter<aalwines::State<aalwines::Query::mode_t::EXACT>> {
        using T = aalwines::State<aalwines::Query::mode_t::EXACT>;

        static constexpr size_t size(const T& data) {
            return byte_vector_converter<aalwines::State<aalwines::Query::mode_t::OVER>>::size(data._s) +
                   byte_vector_converter<aalwines::EdgeStatus>::size(data._edge_state);
        }
        static constexpr void push_back_bytes(std::vector<std::byte>& result, const T& data) {
            byte_vector_converter<aalwines::State<aalwines::Query::mode_t::OVER>>::push_back_bytes(result, data._s);
            byte_vector_converter<aalwines::EdgeStatus>::push_back_bytes(result, data._edge_state);
        }
        static constexpr void from_bytes(const std::vector<std::byte>& bytes, size_t& bytes_id, T& data) {
            byte_vector_converter<aalwines::State<aalwines::Query::mode_t::OVER>>::from_bytes(bytes, bytes_id, data._s);
            byte_vector_converter<aalwines::EdgeStatus>::from_bytes(bytes, bytes_id, data._edge_state);
        }
    };
}

namespace aalwines {

    // This one is general, and will be reused by both NetworkPDAFactory and CegarNetworkPDAFactory
    template<Query::mode_t mode>
    class NetworkTranslation {
    protected:
        using label_t = Query::label_t;
        using NFA = pdaaal::NFA<label_t>;
        using nfa_state_t = NFA::state_t;
        using state_t = State<mode>;
    public:
        NetworkTranslation(const Query& query, const Network& network) : _query(query), _network(network) { };

        void make_initial_states(const std::function<void(state_t&&)>& add_initial) {
            auto add = [&add_initial](const std::vector<nfa_state_t*>& next, const Interface* inf) {
                if (inf != nullptr && inf->is_virtual()) return; // don't start on a virtual interface.
                for (const auto& n : next) {
                    add_initial(state_t(inf, n));
                }
            };
            for (const auto& i : _query.path().initial()) {
                for (const auto& e : i->_edges) {
                    auto next = e.follow_epsilon();
                    if (e.wildcard(_network.all_interfaces().size())) {
                        for (const auto& inf : _network.all_interfaces()) {
                            add(next, inf->match());
                        }
                    } else if (!e._negated) {
                        for (const auto& s : e._symbols) {
                            auto inf = _network.all_interfaces()[s];
                            add(next, inf->match());
                        }
                    } else {
                        for (const auto& inf : _network.all_interfaces()) {
                            if (e.contains(inf->global_id())) {
                                add(next, inf->match());
                            }
                        }
                    }
                }
            }
        }

        void rules(const state_t& from_state,
                   const std::function<void(state_t&&, const RoutingTable::entry_t&, const RoutingTable::forward_t&)>& add_rule_type_a,
                   const std::function<void(state_t&&, const label_t&, std::pair<pdaaal::op_t,label_t>&&)>& add_rule_type_b) {
            if (from_state.ops_done()) {
                const auto& entries = from_state.interface()->table()->entries();
                for (size_t eid = 0; eid < entries.size(); ++eid) {
                    const auto& entry = entries[eid];
                    for (size_t rid = 0; rid < entry._rules.size(); ++rid) {
                        const auto& forward = entry._rules[rid];
                        std::optional<EdgeStatus> next_edge_status;
                        if constexpr (mode == Query::mode_t::EXACT) {
                            next_edge_status = from_state.next_edge_state(entry, forward, _query.number_of_failures());
                            if (!next_edge_status.has_value()) continue;
                        } else if constexpr (mode == Query::mode_t::OVER) {
                            if (!state_t::is_okay(entry, forward, _query.number_of_failures())) continue;
                        } else {
                            assert(false); // Unexpected Query::mode_t
                        }
                        if (forward._via->is_virtual()) { // Virtual interface does not use a link, so keep same NFA state.
                            if constexpr (mode == Query::mode_t::EXACT) {
                                add_rule_type_a((forward._ops.size() <= 1)
                                    ? state_t(forward._via->match(), from_state.nfa_state(), std::move(next_edge_status).value())
                                    : state_t(eid, rid, 0, from_state.interface(), from_state.nfa_state(), std::move(next_edge_status).value()), entry, forward);
                            } else {
                                add_rule_type_a((forward._ops.size() <= 1)
                                    ? state_t(forward._via->match(), from_state.nfa_state())
                                    : state_t(eid, rid, 0, from_state.interface(), from_state.nfa_state()), entry, forward);
                            }
                            //apply_per_nfastate(from_state._nfa_state);
                        } else { // Follow NFA edges matching forward._via
                            for (const auto& e : from_state.nfa_state()->_edges) {
                                if (!e.contains(forward._via->global_id())) continue;
                                for (const auto& n : e.follow_epsilon()) {
                                    if constexpr (mode == Query::mode_t::EXACT) {
                                        add_rule_type_a((forward._ops.size() <= 1)
                                                        ? state_t(forward._via->match(), n, next_edge_status.value())
                                                        : state_t(eid, rid, 0, from_state.interface(), n, next_edge_status.value()), entry, forward);
                                    } else {
                                        add_rule_type_a((forward._ops.size() <= 1)
                                                        ? state_t(forward._via->match(), n)
                                                        : state_t(eid, rid, 0, from_state.interface(), n), entry, forward);
                                    }
                                    //apply_per_nfastate(n);
                                }
                            }
                        }

                    }
                }
            } else {
                const auto& forward = from_state.forward();
                assert(from_state.opid() + 1 < forward._ops.size()); // Otherwise we would already have moved to the next interface.

                auto pre_label = (forward._ops[from_state.opid()]._op == RoutingTable::op_t::POP)
                        ? Query::wildcard_label()
                        : forward._ops[from_state.opid()]._op_label;
                add_rule_type_b(state_t::perform_op(from_state, forward), // To-state
                                pre_label,
                                forward._ops[from_state._opid + 1].convert_to_pda_op()); // Op, op-label
            }
        }

        size_t set_approximation(const State& state, const RoutingTable::forward_t& forward) {
            if (forward._via->is_virtual()) return state._appmode;
            auto num_fail = _query.number_of_failures();
            auto err = std::numeric_limits<size_t>::max();
            switch (_query.approximation()) {
                case Query::OVER:
                    return (forward._priority > num_fail) ? err : 0; // TODO: This is incorrect. Should be size of set of links for forwards with smaller priority on current entry...
                case Query::UNDER: {
                    auto nm = state._appmode + forward._priority; // TODO: Also incorrect. Same.
                    return (nm > num_fail) ? err : nm;
                }
                case Query::EXACT:
                case Query::DUAL:
                default:
                    return err;
            }
        }

        static void add_link_to_trace(json& trace, const Interface* inf, const std::vector<label_t>& final_header) {
            trace.emplace_back();
            trace.back()["from_router"] = inf->target()->name();
            trace.back()["from_interface"] = inf->match()->get_name();
            trace.back()["to_router"] = inf->source()->name();
            trace.back()["to_interface"] = inf->get_name();
            trace.back()["stack"] = json::array();
            std::for_each(final_header.rbegin(), final_header.rend(), [&stack=trace.back()["stack"]](const auto& label){
                if (label == Query::bottom_of_stack()) return;
                std::stringstream s;
                s << label;
                stack.emplace_back(s.str());
            });
        }

    private:
        const Query& _query;
        const Network& _network;
    };

    template<Query::mode_t mode, typename W_FN = std::function<void(void)>>
    class NetworkTranslationW : public NetworkTranslation<mode> {
        using weight_type = typename W_FN::result_type;
        static constexpr bool is_weighted = pdaaal::is_weighted<weight_type>;
    public:
        NetworkTranslationW(const Query& query, const Network& network, const W_FN& weight_f)
        : NetworkTranslation<mode>(query, network), _weight_f(weight_f) { };

        void add_rule_to_trace(json& trace, const Interface* inf, const RoutingTable::entry_t& entry, const RoutingTable::forward_t& rule) const {
            trace.emplace_back();
            trace.back()["ingoing"] = inf->get_name();
            std::stringstream s;
            if (entry.ignores_label()) {
                s << "null";
            } else {
                s << entry._top_label;
            }
            trace.back()["pre"] = s.str();
            trace.back()["rule"] = rule.to_json();
            if constexpr (is_weighted) {
                trace.back()["priority-weight"] = json::array();
                auto weights = _weight_f(rule, entry);
                for (const auto& w : weights){
                    trace.back()["priority-weight"].push_back(std::to_string(w));
                }
            }
        }

    private:
        const W_FN& _weight_f;
    };

}

#endif //AALWINES_NETWORKTRANSLATION_H

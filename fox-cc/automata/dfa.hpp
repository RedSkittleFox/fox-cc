#pragma once

#include <unordered_map>
#include <vector>
#include <set>
#include <optional>
#include <algorithm>

#include <automata/edge.hpp>
#include <automata/nfa.hpp>

#include <iostream>

namespace fox_cc
{
	namespace automata
	{
		template
		<
			class Value,
			class Reduce,
			class Edge,
			class EdgeTraits = ::fox_cc::automata::edge_traits<Edge>
		>
		using dfa = state_machine<Value, Reduce, Edge, EdgeTraits, true, true>;

		template
		<
			class Value,
			class Reduce,
			class Edge,
			class EdgeTraits = ::fox_cc::automata::edge_traits<Edge>
		>
		dfa<Value, Reduce, Edge, EdgeTraits> construct_dfa
		(
			const nfa<Value, Reduce, Edge, EdgeTraits>& nfa, 
			std::invocable<const Reduce&, const Reduce&> auto&& reduce_conflict_resolver,
			std::invocable<const Value&, const Value&> auto&& value_merge_conflict_resolver
		)
		requires
			std::is_invocable_r_v<Reduce, decltype(reduce_conflict_resolver), const Reduce&, const Reduce&>
		&&
			std::is_invocable_r_v<Value, decltype(value_merge_conflict_resolver), const Value&, const Value&>
		{
#if 0
			for(size_t i = 0; i < nfa.size(); ++i)
			{
				std::cout << i << '\n';

				for(auto to : nfa[i].next())
				{
					if constexpr(std::is_same_v<const std::bitset<128>, decltype(to.first)>)
					{
						std::cout << "\t" << to.second << " | ";
						if(to.first.all())
						{
							std::cout << "EPSILON";
						}
						else
						{
							for (size_t i = 0; i < 128; ++i)
							{
								if (to.first.test(i))
								{
									std::cout << static_cast<char>(i) << ' ';
								}
							}
						}
						
						std::cout << '\n';
					}
					else
					{
						std::cout << "\t" << to.second << " | " << to.first << '\n';
					}
				}
			}
#endif
			// Uses power-set algorithm to convert NFA to DFA
			dfa<Value, Reduce, Edge, EdgeTraits> out;

			// Compute epsilon closure sets - states that we can go by following epsilons from a given state
			std::vector<std::set<size_t>> epsilon_closure_sets(nfa.size()); // maps state to it's closure set

			{
				// Generate immediate epsilon transitions
				for (size_t i = 0; i < std::size(epsilon_closure_sets); ++i)
				{
					epsilon_closure_sets[i].insert(i);

					auto& state = nfa[i];
					auto [first, last] = state.next().equal_range(EdgeTraits::epsilon());
					for (auto to : std::ranges::subrange(first, last) | std::views::values)
						epsilon_closure_sets[i].insert(to);
				}

				// A closure algorithm
				bool changed = false;
				do
				{
					changed = false;

					for(size_t i = 0; i < std::size(epsilon_closure_sets); ++i)
					{
						const size_t start_size = std::size(epsilon_closure_sets[i]);

						std::set<size_t> new_set;

						for(auto e : epsilon_closure_sets[i])
						{
							new_set.merge(std::set<size_t>(epsilon_closure_sets[e]));
						}

						epsilon_closure_sets[i] = std::move(new_set);

						changed = changed || (start_size != std::size(epsilon_closure_sets[i]));
					}

				} while (changed);
			}

			{
				std::map<std::set<size_t>, size_t> from_transition_states;
				std::map<size_t, std::set<size_t>> to_transition_states;

				// init first state
				{
					size_t s = out.insert();
					assert(s == 0);
					from_transition_states[epsilon_closure_sets[s]] = s;
					to_transition_states[s] = epsilon_closure_sets[s];
					out.start() = s;
				}

				for(size_t i = 0; i < out.size(); ++i)
				{
					using edge_traits = typename decltype(out)::edge_traits;

					// Compute DFA edges
					std::vector<Edge> edges;
					for(
						const auto& to_states = to_transition_states[i]; 
						auto state : to_states
						)
					{
						edges.insert_range(std::end(edges), nfa[state].next() | std::views::keys 
							| std::views::filter([](const auto& edge) -> bool {return edge != edge_traits::epsilon(); }));
					}

					edges = edge_traits::unique_edges(edges); // compute DFA edges

					// Create edges and states
					for(const auto& e : edges)
					{
						std::set<size_t> closurefied_states;
						{
							// compute states we can go to by the edge e
							std::set<size_t> states;

							for (
								const auto& to_states = to_transition_states[i];
								auto state : to_states
								)
							{
								for (const auto& [edge, next_state] : nfa[state].next())
								{
									if (edge == edge_traits::epsilon())
										continue;

									if (!edge_traits::empty_intersection(edge, e))
										states.insert(next_state);
								}
							}

							// compute epsilon closure set of states
							
							for (auto state : states)
							{
								closurefied_states.insert(state);
								closurefied_states.insert_range(epsilon_closure_sets[state]);
							}
						}

						size_t new_state_id;

						// Check if that state already exists
						auto r = from_transition_states.find(closurefied_states);

						if(r == std::end(from_transition_states))
						{
							new_state_id = out.insert();
							assert(new_state_id == out.size() - 1);
							from_transition_states[closurefied_states] = new_state_id;
							to_transition_states[new_state_id] = closurefied_states;

							const auto& nfa_state = nfa[*closurefied_states.begin()];

							// Resolve reduction and value
							std::optional<Reduce> reduce = nfa_state.reduce();
							Value value = nfa_state.value();

							for(auto& nfa_state_id : closurefied_states | std::views::drop(1))
							{
								const auto& nfa_state = nfa[nfa_state_id];

								if(nfa_state.reduce())
								{
									if(reduce)
									{
										reduce = reduce_conflict_resolver(nfa_state.reduce().value(), reduce.value());
									}
									reduce = nfa_state.reduce();
								}

								value = value_merge_conflict_resolver(value, nfa_state.value());
							}

							out[new_state_id].reduce() = std::move(reduce);
							out[new_state_id].value() = std::move(value);
						}
						else
						{
							new_state_id = r->second;
						}

						// Add edge from s
						out.connect(i, new_state_id, e);
					}
				}
			}

			return out;
		}

		template
		<
			class Value,
			class Reduce,
			class Edge,
			class EdgeTraits = ::fox_cc::automata::edge_traits<Edge>
		>
		dfa<Value, Reduce, Edge, EdgeTraits> construct_dfa(const nfa<Value, Reduce, Edge, EdgeTraits>& nfa)
		{
			return construct_dfa<Value, Reduce, Edge, EdgeTraits>(nfa, [](const Reduce& lhs, const Reduce& rhs) -> Reduce { return std::min(lhs, rhs); }, [](const Value& lhs, const Value& rhs) -> Value { return lhs; });
		}
	}
}
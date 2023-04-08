#include "compiler.hpp"

void cmp::compiler::generate_production_table()
{
	if (std::empty(ast_.productions))
		error("No productions.");

	// Pick starting production
	if (ast_.start_identifier)
	{
		start_production_
			= token_id_by_name(ast_.start_identifier.info->string_value);
	}
	else
	{
		start_production_
			= token_id_by_name(ast_.productions.front().name.info->string_value);
	}

	// Generate all producitons
	for (auto& prod : ast_.productions)
	{
		auto name
			= token_id_by_name(prod.name.info->string_value);

		for (auto& rule : prod.rules)
		{
			std::vector<production_symbol_t> rules;

			for (size_t i{}; i < std::size(rule); ++i)
			{
				if (auto v = rule[i].value;
					v == prs::token::IDENTIFIER || v == prs::token::LITERAL)
				{
					rules.push_back({ token_id_by_name(rule[i].info->string_value) });
				}
				else if (v == prs::token::C_ACTION) // Attach action
				{
					// Action has to be attached to some matched statement
					if (std::empty(rules))
						error("Invalid c-action");

					rules.back().action_id = std::size(actions_);
					actions_.emplace_back(rule[i]);
				}
				else
				{
					error("");
				}
			}

			// Optimize memory
			rules.shrink_to_fit();

			productions_.insert(std::make_pair(name, std::move(rules)));
		}
	}
}

void cmp::compiler::generate_first_sets()
{
	bool mutated = false;
	do
	{
		mutated = false;

		for (const auto& [name, prod] : productions_)
		{
			auto& first_set = first_sets_[name];
			const size_t old_size = std::size(first_set);

			if (std::empty(prod))
			{
				first_set.insert(0); // end-symbol
			}
			else if (productions_.contains(prod[0])) // if non-terminal
			{
				auto s = first_sets_[prod[0]];
				s.erase(0);
				first_set.merge(std::move(s));
			}
			else if ((prod[0] == 0 && std::size(prod) == 1) || prod[0] != 0) // if terminal
			{
				first_set.insert(prod[0]);
			}

			mutated = mutated || (old_size != std::size(first_set));
		}

	} while (mutated);
}

void cmp::compiler::generate_states()
{
	auto r = productions_.equal_range(start_production_);
	auto sr = std::ranges::subrange(r.first, r.second);

	state s;
	state_insert_productions(s, start_production_);
	states_.push_back(std::move(s));
	closure_populate_state(0);
	closure_generate_follow_sets(0);
	std::vector<size_t> states_to_process{ 0 };


	for (size_t i = {}; i < states_to_process.size(); ++i)
	{
		const size_t state_id = states_to_process[i];
		auto v = generate_gotos(state_id);

		for (auto id : v)
		{
			closure_populate_state(id);
			closure_generate_follow_sets(id);
		}

		// Merge already existing states to avoid cyclic dependency
		merge_states(state_id, v);
		states_to_process.insert_range(std::end(states_to_process), v);
		// this->debug_print();
	}
}

bool cmp::compiler::closure_populate_state(size_t state_id)
{
	bool modified_once = false;
	bool modified;

	assert(state_id < std::size(states_));

	auto& s = states_[state_id];

	do
	{
		modified = false;

		for (const auto& prod : s.productions | std::views::keys)
		{
			for (const auto t : prod.prod)
			{
				if (modified = state_insert_productions(s, t); modified)
				{
					// above potentially invalidates iterators, that's why we enter the loop again
					goto break_outer;
				}
			}
		}

	break_outer:
		modified_once = modified_once || modified;
	} while (modified);

	return modified_once;
}

bool cmp::compiler::closure_generate_follow_sets(size_t state_id)
{
	bool modified_once = false;
	bool modified = false;

	assert(state_id < std::size(states_));

	auto& s = states_[state_id];

	std::map<non_terminal_id_t, std::set<terminal_id_t>> follow_sets;

	// Populate follow sets with current follows
	for (const auto& [prod, follow] : s.productions)
		follow_sets[prod.name] = follow;

	do
	{
		modified = false;

		for (const auto& prod : s.productions | std::views::keys)
		{
			if (std::empty(prod.prod))
				continue;

			auto R = prod.prod[0];

			// R is a terminal, abort
			if (!productions_.contains(R))
				continue;

			auto& follow_set = follow_sets[R];

			const auto old_size = std::size(follow_set);

			// ... = ... . R a -> can be followed by a
			if (std::size(prod.prod) >= 2 && !productions_.contains(prod.prod[1]))
			{
				modified = modified || follow_set.insert(prod.prod[1]).second;
			}
			// ... = ... R A -> can be followed by FIRST(A)
			else if (std::size(prod.prod) >= 2 && productions_.contains(prod.prod[1]))
			{
				follow_set.insert_range(first_sets_[prod.prod[1]]);
			}
			// A = ... . R -> can be followed by FOLLOW(A)
			else if (std::size(prod.prod) == 1)
			{
				follow_set.insert_range(follow_sets[prod.name]);
			}

			modified = modified || (old_size != std::size(follow_set));
		}

		modified_once |= modified;
	} while (modified);

	// Apply follow sets
	for (auto& [prod, follow] : s.productions)
	{
		// Only apply them to non-canonical productions
		if (!std::empty(prod.stack))
			continue;

		follow = follow_sets[prod.name];
	}

	return modified_once;
}

std::vector<size_t> cmp::compiler::generate_gotos(size_t state_id)
{
	std::vector<size_t> out_states;

	assert(state_id < states_.size());
	auto* s = std::addressof(states_[state_id]);

	// Generate possible following tokens
	std::set<token_id_t> followers;

	for (auto& prod : s->productions | std::views::keys)
	{
		if (!std::empty(prod.prod))
			followers.insert(prod.prod[0]);
	}

	for (auto f : followers)
	{
		state out_s;

		for (auto& [prod, follow] : s->productions)
		{
			if (!std::empty(prod.prod))
			{
				if (prod.prod[0] == f)
				{
					state::production out_p;
					out_p.name = prod.name;
					out_p.prod = prod.prod.subspan(1);

					if (std::empty(prod.stack))
					{
						out_p.stack = prod.prod.subspan(0, 1);
					}
					else
					{
						// HACK: We know we own this memory! 
						out_p.stack = std::span(std::data(prod.stack), std::size(prod.stack) + 1);
					}

					out_s.productions.insert(std::make_pair(out_p, follow));
				}
			}
		}

		if (!std::empty(out_s.productions))
		{
			out_states.push_back(std::size(states_));
			states_.push_back(out_s);
			s = std::addressof(states_[state_id]); // Fix dangling iterator after push-back
			s->transitions[f] = out_states.back();
		}
	}

	return out_states;
}

void cmp::compiler::merge_states(size_t source_state, std::vector<size_t>& new_states)
{
	auto og_ids = new_states;
	std::vector<size_t> erase_list;

	auto& s = states_[source_state];

	for (auto& cs : new_states)
	{
		auto& c = states_[cs];

		auto r = std::ranges::find_if(states_, [&](const state& other) -> bool
			{
				return other.productions == c.productions;
			}
		);

		if (r != std::end(states_))
		{
			const size_t dup_id = std::distance(std::begin(states_), r);

			if (dup_id == cs)
				continue;

			erase_list.push_back(cs);

			// Fix up connection
			for (auto& t : s.transitions)
			{
				if (t.second == cs)
				{
					t.second = dup_id;
					break;
				}
			}

			// Mark for erasure
			cs = std::numeric_limits<size_t>::max();
		}
	}

	if (std::erase(new_states, std::numeric_limits<size_t>::max()) == 0)
		return;

	// Fix state ids after the shift (removal of some states)
	for (size_t i = 0; i < std::size(new_states); ++i)
	{
		auto& old_id = new_states[i];
		auto new_id = og_ids[i];

		for (auto& t : s.transitions)
		{
			if (t.second == old_id)
			{
				t.second = new_id;
				break;
			}
		}

		old_id = new_id;
	}

	// Erase states that are duplicated
	for (auto e : erase_list | std::views::reverse)
	{
		states_.erase(std::begin(states_) + e);
	}
}

bool cmp::compiler::state_insert_productions(state& s, non_terminal_id_t production)
{
	auto r = productions_.equal_range(production);
	auto sr = std::ranges::subrange(r.first, r.second);

	bool modified = false;

	for (const auto& p : sr)
	{
		state::production prod
		{
			.name = p.first,
			.prod = p.second
		};

		const auto old_size = std::size(s.productions);
		(void)s.productions[prod];

		modified = modified || std::size(s.productions) != old_size;
	}

	return modified;
}
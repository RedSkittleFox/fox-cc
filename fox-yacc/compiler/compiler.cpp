#include "compiler.hpp"

void cmp::compiler::generate_char_tokens()
{
	// Generate char-types
	for (size_t i = 0; i <= std::numeric_limits<char>::max(); ++i)
	{
		auto name = std::string("'") +
			(std::isalnum(static_cast<int>(i)) ? static_cast<char>(i) : '?') + "'";

		tokens_[i] =
		{
			.type = p_default_t_,
			.value = i,
			.name = std::move(name),
			.terminal = true
		};
	}
}

void cmp::compiler::generate_terminal_tokens()
{
	// Generate terminals
	for (auto& def : ast_.token_definitions)
	{
		// TODO: Deduce assoc

		// Deduce type
		const std::string* type = p_default_t_;

		if (static_cast<bool>(def.tag))
		{
			auto r = types_.insert(std::string(def.tag.info->string_value)).first;
			type = std::addressof(*r);
		}

		// Iterate over terminals

		for (auto& term : def.tokens)
		{
			assert_unique_name(term.name);

			size_t pos;

			if (term.number)
			{
				const int v = term.number.info->int_value;
				if (v < 128 || v >= 512)
					error("Manual token id out of range.");

				auto& e = tokens_[v];

				if (e.type != nullptr)
					error("Manual token id already in use.");

				pos = v;
			}
			else
			{
				if (std::empty(token_free_list_))
					error("No token id's available.");

				pos = token_free_list_.back();
				token_free_list_.pop_back();
			}

			auto& e = tokens_[pos];

			e.type = type;
			e.name = term.name.info->string_value;
			e.value = pos;
			e.terminal = true;
		}
	}
}

void cmp::compiler::generate_production_tokens()
{
	// Because use is allowed before definition/declaration of the production, 
	// we need to first register all productions

	for (auto& prod : ast_.productions)
	{
		assert_unique_name(prod.name);

		if (std::empty(token_free_list_))
			error("No token id's available.");

		const size_t pos = token_free_list_.back();
		token_free_list_.pop_back();

		auto& e = tokens_[pos];

		const std::string* type = p_default_t_;

		if (auto r = ast_.name_type.find(prod.name); r != std::end(ast_.name_type))
		{
			auto l = types_.insert(std::string(r->second.info->string_value)).first;
			type = std::addressof(*l);
		}

		e.type = type;
		e.name = prod.name.info->string_value;
		e.value = pos;
		e.terminal = false;
	}
}

void cmp::compiler::optimize_token_table()
{
	auto r = std::find_if(std::rbegin(tokens_), std::rend(tokens_),
		[](const token& t) -> bool {return t.type != nullptr; });

	// TODO: Test
	const auto pos = std::distance(r, std::rend(tokens_));
	tokens_.erase(std::begin(tokens_) + pos, std::end(tokens_));
	tokens_.shrink_to_fit();

	token_free_list_.clear();
	token_free_list_.shrink_to_fit();
}

void cmp::compiler::generate_production_table()
{
	if (std::empty(ast_.productions))
		error("No productions.");

	if(ast_.start_identifier)
	{
		start_production_
			= token_id_by_name(ast_.start_identifier.info->string_value);
	}
	else
	{
		start_production_
			= token_id_by_name(ast_.productions.front().name.info->string_value);
	}

	// TODO: Include code segments

	for(auto& prod : ast_.productions)
	{
		auto name
			= token_id_by_name(prod.name.info->string_value);

		for(auto& rule : prod.rules)
		{
			std::vector<size_t> rules;

			for(size_t i{}; i < std::size(rule.tokens); ++i)
			{
				if(auto v = rule.tokens[i].value; 
					v == prs::token::IDENTIFIER || v == prs::token::LITERAL)
				{
					rules.push_back(token_id_by_name(rule.tokens[i].info->string_value));
				}
			}

			// Don't let them default to std::slice_array<T>, it's evil!
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

		for(const auto& [name, prod] : productions_)
		{
			auto& first_set = first_sets_[name];
			const size_t old_size = std::size(first_set);

			if(std::empty(prod))
			{
				first_set.insert(0); // end-symbol
			}
			else if(productions_.contains(prod[0])) // if non-terminal
			{
				first_set.insert_range(first_sets_[prod[0]]);
			}
			else // if terminal
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

		for(auto id : v)
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

	std::set<state::production> new_productions;

	do
	{
		modified = false;

		for(const auto& prod : s.productions)
		{
			for(const auto t : prod.prod)
			{
				if(state_insert_productions(s, t))
				{
					modified = true;
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

	std::map<size_t, std::set<size_t>> follow_sets;

	do
	{
		modified = false;

		for (const auto& prod : s.productions)
		{
			if (std::empty(prod.prod))
				continue;

			auto R = prod.prod[0];

			// R is a terminal, abort
			if (!productions_.contains(R))
				continue;

			auto& follow_set = follow_sets[R];

			// ... = ... . R a -> can be followed by a
			if(std::size(prod.prod) >= 2 && !productions_.contains(prod.prod[1]))
			{
				modified = modified || follow_set.insert(prod.prod[1]).second;
			}
			// ... = ... R A -> can be followed by FIRST(A)
			else if(std::size(prod.prod) >= 2 && productions_.contains(prod.prod[1]))
			{
				const auto old_size = std::size(follow_set);
				follow_set.insert_range(first_sets_[prod.prod[1]]);
				modified = modified || (old_size != std::size(follow_set));
			}
			// A = ... . R -> can be followed by FOLLOW(A)
			else if(std::size(prod.prod) == 1)
			{
				const auto old_size = std::size(follow_set);
				follow_set.insert_range(follow_sets[prod.name]);
				modified = modified || (old_size != std::size(follow_set));
			}
		}

		modified_once |= modified;
	} while (modified);

	// Apply follow sets
	for (const auto& prod : s.productions)
	{
		if (!std::empty(prod.stack))
			continue;

		prod.follow.insert_range(follow_sets[prod.name]);
	}

	return modified_once;
}

std::vector<size_t> cmp::compiler::generate_gotos(size_t state_id)
{
	std::vector<size_t> out_states;

	assert(state_id < states_.size());
	auto* s = std::addressof(states_[state_id]);

	// Generate possible following tokens
	std::set<size_t> followers;

	for (auto& prod : s->productions)
	{
		if (!std::empty(prod.prod))
			followers.insert(prod.prod[0]);
	}

	for (auto f : followers)
	{
		struct state out_s;

		for (auto& prod : s->productions)
		{
			if (!std::empty(prod.prod))
			{
				if (prod.prod[0] == f)
				{
					state::production out_p;
					out_p.name = prod.name;
					out_p.follow = prod.follow;
					out_p.prod = prod.prod.subspan(1);

					if(std::empty(prod.stack))
					{
						out_p.stack = prod.prod.subspan(0, 1);
					}
					else
					{
						// HACK: We know we own this memory! 
						out_p.stack = std::span(std::data(prod.stack), std::size(prod.stack) + 1);
					}

					out_s.productions.insert(std::move(out_p));
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

	for(auto& cs : new_states)
	{
		auto& c = states_[cs];
		auto r = std::ranges::find_if(states_, [c](const state& s) -> bool
			{
				return std::ranges::equal(c.productions, s.productions,
					[](const state::production& lhs, const state::production& rhs) -> bool
					{
						return
							lhs.name == rhs.name &&
							std::ranges::equal(lhs.stack, rhs.stack) &&
							std::ranges::equal(lhs.prod, rhs.prod) &&
							lhs.follow == rhs.follow;
					}
				);
			}
		);

		if(r != std::end(states_))
		{
			const size_t dup_id = std::distance(std::begin(states_), r);

			if (dup_id == cs)
				continue;

			erase_list.push_back(cs);

			// Fix up connection
			for(auto& t : s.transitions)
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
	for(size_t i = 0; i < std::size(new_states); ++i)
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
	for(auto e : erase_list | std::views::reverse)
	{
		states_.erase(std::begin(states_) + e);
	}
}

bool cmp::compiler::state_insert_productions(state& s, size_t production, const std::set<size_t>& followers)
{
	auto r = productions_.equal_range(production);
	auto sr = std::ranges::subrange(r.first, r.second);

	bool modified = false;

	for (const auto& p : sr)
	{
		state::production prod
		{
			.name = p.first,
			.prod = p.second,
			.follow = followers
		};

		const auto r = s.productions.find(prod);

		if(r == std::end(s.productions))
		{
			modified = true;
			s.productions.insert(std::move(prod));
		}
		else
		{
			const auto old_size = std::size(r->follow);
			r->follow.insert_range(followers);

			// Check if insertion changed anything
			modified = modified || (old_size != std::size(r->follow));
		}
	}

	return modified;
}

cmp::compiler::token* cmp::compiler::token_by_name(std::string_view sv)
{
	auto r = std::ranges::find_if(tokens_,
		[=](const token& t)->bool { return t.name == sv; });

	if (r == std::end(tokens_))
		error("Invalid token name");

	return std::addressof(*r);
}

size_t cmp::compiler::token_id_by_name(std::string_view sv)
{
	const auto r = token_by_name(sv);

	if (r == nullptr)
		return std::numeric_limits<size_t>::max();

	return r->value;
}

void cmp::compiler::debug_print()
{
	for(size_t i{}; i < states_.size(); ++i)
	{
		auto& s = states_[i];

		std::cout << "====== " << i << " ======\n";

		for(auto& prod : s.productions)
		{
			std::cout << tokens_[prod.name].name << " -> ";

			for (auto stack : prod.stack)
				std::cout << tokens_[stack].name;

			std::cout << ".";

			for (auto p : prod.prod)
				std::cout << tokens_[p].name;

			std::cout << " { ";

			for (auto f : prod.follow)
				if (f != 0)
					std::cout << tokens_[f].name;
				else std::cout << "$";

			std::cout << " }\n";

			
		}

		for (auto t : s.transitions)
		{
			std::cout << "[ " << tokens_[t.first].name << " => " << t.second << " ]\n";
		}

		std::cout << "\n";
	}
}

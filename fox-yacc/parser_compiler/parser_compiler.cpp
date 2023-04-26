#include <functional>
#include <parser_compiler/parser_compiler.hpp>

fox_cc::parser_compiler::parser_compiler(const lex_compiler::lex_compiler_result& lex_result, const prs::yacc_ast& ast)
	: lex_result_(lex_result), ast_(ast)
{
	init_terminals();
	init_non_terminals();
	generate_first_sets();
	init_first_state();
	init_states();
}

void fox_cc::parser_compiler::init_terminals()
{
	result_.tokens.clear();

	for(const auto& t : lex_result_.terminals)
	{
		parser_compiler_result::terminal v;
		v.name = t.name;
		v.id = std::size(result_.tokens);
		v.assoc = t.assoc;

		result_.tokens.emplace_back(v);
	}
}

void fox_cc::parser_compiler::init_non_terminals()
{
	first_non_terminal_ = std::size(result_.tokens);

	// Register non-terminals
	for(const auto& p : ast_.productions)
	{
		parser_compiler_result::non_terminal v;
		v.name = p.name.info->string_value;
		v.id = std::size(result_.tokens);

		if(this->token_by_name(v.name) != parser_compiler_result::token_id_npos)
		{
			error("Token already defined.");
		}

		result_.tokens.emplace_back(v);
	}

	// Parse productions
	for(size_t i = first_non_terminal_, j = 0; i < std::size(result_.tokens); ++i, ++j)
	{
		auto& production = result_.tokens[i].non_terminal();
		auto& ast_production = ast_.productions[j];

		for(const auto& rule : ast_production.rules)
		{
			auto& prod = production.productions.emplace_back();

			for(const auto& token : rule)
			{
				auto token_id = token_by_name(std::string(token.info->string_value));

				if(token_id == parser_compiler_result::token_id_npos)
				{
					error("Unknown token in production");
				}

				prod.push_back(token_id);
			}

			prod.shrink_to_fit();
		}
	}
}

void fox_cc::parser_compiler::generate_first_sets()
{
	first_sets_.resize(std::size(result_.tokens) - first_non_terminal_);

	for(bool modified = true; modified == true; )
	{
		modified = false;

		for(const auto& nt : result_.tokens | std::views::drop(first_non_terminal_) 
			| std::views::transform(std::mem_fn(
				static_cast<const parser_compiler_result::non_terminal& (parser_compiler_result::token::*)() const noexcept>(&parser_compiler_result::token::non_terminal)
			))
			)
		{
			auto& set = first_set(nt.id);
			const size_t set_size = std::size(set);

			for(auto& prod : nt.productions)
			{
				if (std::empty(prod))
					continue;

				auto& token = result_.tokens[prod.front()];

				if(token.is_terminal())
				{
					set.insert(token.id());
				}
				else
				{
					set.insert_range(first_set(token.id()));
				}
			}

			modified = modified || std::size(set) != set_size;
		}
	}
}

void fox_cc::parser_compiler::init_first_state()
{
	size_t start_production = first_non_terminal_;

	if(ast_.start_identifier.info != nullptr)
	{
		start_production = token_by_name(std::string(ast_.start_identifier.info->string_value));
	}

	auto& first_state = result_.dfa[result_.dfa.insert()].value();

	insert_production(first_state, start_production, { parser_compiler_result::end_token });
	populate_state(first_state);
}

void fox_cc::parser_compiler::init_states()
{
	for(size_t i = 0; i < std::size(result_.dfa); ++i)
	{
		auto* dfa_state = std::addressof(result_.dfa[i]);
		auto* state = std::addressof(dfa_state->value());

		// Generate goto states

		std::set<parser_compiler_result::token_id> edge_tokens;

		// Find edges
		for(auto& prod : state->productions)
		{
			const auto& source_production = result_.tokens[prod.non_terminal].non_terminal().productions[prod.non_terminal_production];

			if(prod.current < std::size(source_production))
			{
				edge_tokens.insert(source_production[prod.current]);
			}
		}

		// Generate new states
		for(auto edge : edge_tokens)
		{
			parser_compiler_result::state_data goto_state;

			for (auto& prod : state->productions)
			{
				const auto& source_production = result_.tokens[prod.non_terminal].non_terminal().productions[prod.non_terminal_production];

				if (prod.current < std::size(source_production) && source_production[prod.current] == edge)
				{
					auto& j = goto_state.productions.emplace_back(prod);
					j.current += 1;
				}
			}

			populate_state(goto_state);

			std::optional<size_t> goto_state_id;

			// Check if state already exists
			for(size_t j = 0; j < std::size(result_.dfa); ++j)
			{
				if(result_.dfa[j].value() == goto_state)
				{
					goto_state_id = j;
					break;
				}
			}

			if(!goto_state_id)
			{
				goto_state_id = result_.dfa.insert(goto_state);
			}

			dfa_state = std::addressof(result_.dfa[i]);
			state = std::addressof(dfa_state->value());

			result_.dfa.connect(i, goto_state_id.value(), edge);
		}
	}
}

void fox_cc::parser_compiler::populate_state(parser_compiler_result::state_data& state)
{
	size_t core = std::size(state.productions);

	for(bool changed = true; changed == true; )
	{
		changed = false;

		// Iterate over all productions
		for (size_t i = 0; i < std::size(state.productions); ++i)
		{
			auto& current_production = state.productions[i];
			const auto& source_production = result_.tokens[current_production.non_terminal].non_terminal().productions[current_production.non_terminal_production];

			// If current symbol in a production is a non-terminal
			if (current_production.current < std::size(source_production) && result_.tokens[source_production[current_production.current]].is_non_terminal())
			{
				std::set<parser_compiler_result::token_id> follow_r;

				if(current_production.current + 1 < std::size(source_production))
				{
					// ... = ... . R A -> FOLLOW(R) = FIRST(A)
					if(result_.tokens[source_production[current_production.current + 1]].is_non_terminal())
					{
						follow_r.insert_range(first_set(source_production[current_production.current + 1]));
					}
					// ... = ... . R a -> FOLLOW(R) = a
					else if (result_.tokens[source_production[current_production.current + 1]].is_terminal())
					{
						follow_r.insert(source_production[current_production.current + 1]);
					}
				}
				// A == ... . R -> FOLLOW(R) = FOLLOW(A)
				else if(current_production.current + 1 == std::size(source_production))
				{
					follow_r = current_production.follow_set;
				}

				changed = insert_production(state, source_production[current_production.current], follow_r) || changed;
			}
		}
	}

	
}

bool fox_cc::parser_compiler::insert_production(parser_compiler_result::state_data& state, size_t production_id, std::set<size_t> follow_set)
{
	bool result = false;

	const auto& source_production = result_.tokens[production_id].non_terminal();

	for(size_t i = 0; auto& src_prod : source_production.productions)
	{
		// Check if production is already in-place
		for(auto& current_prod : state.productions)
		{
			if(current_prod.non_terminal == source_production.id && current_prod.non_terminal_production == i && current_prod.current == 0)
			{
				const auto old_size = std::size(current_prod.follow_set);
				current_prod.follow_set.insert_range(follow_set);
				result = result || old_size != std::size(current_prod.follow_set);
				goto next_production;
			}
		}

		// Not found
		state.productions.push_back({
			.non_terminal = production_id,
			.non_terminal_production = i,
			.current = 0,
			.follow_set = follow_set
		});

		result = true;

	next_production:;
		i += 1;
	}

	return result;

}

fox_cc::parser_compiler::parser_compiler_result::token_id fox_cc::parser_compiler::token_by_name(const std::string& name) const noexcept
{
	for(size_t i = 0; const auto& v : result_.tokens)
	{
		if (v.name() == name)
			return i;

		++i;
	}

	return parser_compiler_result::token_id_npos;
}

const std::set<fox_cc::parser_compiler::parser_compiler_result::token_id>& fox_cc::parser_compiler::first_set(
	parser_compiler_result::token_id id) const noexcept
{
	return first_sets_[id - first_non_terminal_];
}

std::set<fox_cc::parser_compiler::parser_compiler_result::token_id>& fox_cc::parser_compiler::first_set(
	parser_compiler_result::token_id id) noexcept
{
	return first_sets_[id - first_non_terminal_];
}

void fox_cc::parser_compiler::error(const char* msg)
{
	assert(false && msg);
}

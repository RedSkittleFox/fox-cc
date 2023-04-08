#include <variant>
#include <memory>
#include <list>
#include <cassert>
#include <functional>

#include "lex_compiler.hpp"

using charset = fox_cc::lex_compiler::charset;

fox_cc::lex_compiler::regex_productions fox_cc::lex_compiler::generate_production(std::string_view regex)
{
	size_t production_id_tracker = 0;

	using production_rule = std::vector<regex_productions::production_token>;
	std::vector<size_t> production_stack;
	std::vector<production_rule*> production_rule_stack;

	regex_productions out;

	auto push_production = [&]() -> void
	{
		production_stack.push_back(production_id_tracker);
		production_rule_stack.push_back(
			std::addressof(out.productions.insert(std::make_pair(production_id_tracker, production_rule{}))->second)
		);
	};

	auto new_production_rule = [&]() -> void
	{
		production_rule_stack.back() =
			std::addressof(out.productions.insert(std::make_pair(production_stack.back(), production_rule{}))->second);
	};

	auto pop_production = [&]() -> void
	{
		size_t last_production = production_stack.back();
		production_stack.pop_back();
		production_rule_stack.pop_back();
		production_rule_stack.back()->push_back({ .production = last_production });
	};

	auto current_rule = [&]() -> production_rule&
	{
		return *production_rule_stack.back();
	};

	auto append = [&]<class T>(T cs) -> void
	{
		if constexpr (std::same_as<T, charset> || std::same_as<T, size_t>)
			current_rule().push_back({ cs });
		else if constexpr (std::same_as<T, char>)
			current_rule().push_back({ charset{}.set(cs) });
	};

	push_production();

	enum special_tokens
	{
		l_parentheses = -64,
		r_parentheses,
		l_brackets,
		r_brackets,
		star,
		plus,
		question_mark,
		logic_or,
		dash
	};

	size_t current_char = 0;

	auto lexer = [&, current_char = static_cast<size_t>(0)](bool minimal = false) mutable -> char
	{
		if (current_char == std::size(regex))
			return '\0';

		char c0 = regex[current_char];
		char c1 = current_char + 1 >= std::size(regex) ? '\0' : regex[current_char + 1];
		current_char += 1;

		switch (c0)
		{
		case '\\':
		{
			switch (c1)
			{
			case '(':
			case ')':
			case '[':
			case ']':
			case '*':
			case '+':
			case '?':
			case '-':
			case '|':
			case '\\':
				return c1;
			case 'n':
				return '\n';
			case 't':
				return '\t';
			case '0':
				return '\0';
			default:
				assert(false);
				break;
			}
			break;
		}
		case '(':
			return minimal ? '(' : l_parentheses;
		case ')':
			return minimal ? ')' : r_parentheses;
		case '[':
			return minimal ? '[' : l_brackets;
		case ']':
			return r_brackets;
		case '*':
			return minimal ? '*' : star;
		case '+':
			return minimal ? '+' : plus;
		case '?':
			return minimal ? '?' : question_mark;
		case '|':
			return minimal ? '?' : logic_or;
		case '-':
			return minimal ? dash : '-';
		default:
			return c0;
		}

		return '\0';
	};

	char c0 = lexer();
	char c1 = lexer();
	char c2 = lexer();

	bool charset_state = false;
	charset charset_set;

	// Parse input
	while(c0 != '\0')
	{
		if(!charset_state)
		{
			switch (c0)
			{
			case l_parentheses:
				push_production();
				break;
			case r_parentheses:
				pop_production();
				break;
			case star:
			case plus:
			case question_mark:
			{
				const regex_productions::production_token
					token = current_rule().back();

				current_rule().pop_back();
				push_production();

				if (c0 == star) // zero or more
				{
					// A -> XA |
					append(token);
					append(production_stack.back());
					new_production_rule();
				}
				else if (c0 == question_mark)
				{
					// A -> X | 
					append(token);
					new_production_rule();
				}
				else if (c0 == plus)
				{
					// A -> XA | X
					append(token);
					append(production_stack.back());
					new_production_rule();
					append(token);
				}
			}
			case l_brackets:
				charset_state = true;
				charset_set.reset();
				break;
			default:
				append(c0);
				break;
			}
		}
		else
		{
			if(c0 == r_brackets)
			{
				append(charset_set);
				charset_state = false;
			}
			else
			{
				if (c1 == dash)
				{
					assert(c0 >= 0 && c2 >= 0);
					assert(c2 != r_brackets && "Mismatched char range");

					for (auto j = static_cast<size_t>(c0); j <= static_cast<size_t>(c2); ++j)
						charset_set.set(j);

					c0 = c1;
					c1 = lexer(true);
					c2 = lexer(true);
				}
				else
				{
					charset_set.set(c0);
				}
			}
			
		}

		// Shift lookups
		c0 = c1; c1 = c2; c2 = lexer(charset_state);
	}

	assert(charset_state == false && std::size(production_stack) == 1);

	this->populate_first_sets(out);
	return out;
}

void fox_cc::lex_compiler::populate_first_sets(regex_productions& prods)
{
	prods.firsts_sets.resize(prods.productions.end()->first);

}
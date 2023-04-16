#pragma once

#include <fox_cc.hpp>
#include <internal_parser/yacc_ast.hpp>

#include <bitset>
#include <map>

namespace fox_cc
{
	class lex_compiler
	{
	public:
		using charset = std::bitset<128>;

	private:
		prs::yacc_ast* ast_ = nullptr;

		struct regex_productions
		{
			union production_token
			{
				charset charset = {};
				size_t production;

				[[nodiscard]] bool is_non_terminal() const noexcept
				{
					return production < 128;
				}

				[[nodiscard]] bool is_terminal() const noexcept
				{
					return production >= 128;
				}
			};

			std::multimap<size_t /*production rule id*/, std::vector<production_token>> productions;
			std::vector<charset> firsts_sets;
		};

	public:
		lexer compile(prs::yacc_ast& ast);

	private:
		regex_productions generate_production(std::string_view regex);
		void populate_first_sets(regex_productions& prods);
	};
}
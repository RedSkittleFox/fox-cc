#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <span>

#include "tokens.hpp"

namespace prs
{
	struct yacc_ast
	{
		// %start name
		token_entry start_identifier;

		// %token [<tag>] name REGEX
		struct definition
		{
			token_entry rword;
			token_entry tag; // optional
			token_entry name;
			token_entry regex;
		};

		std::vector<definition> token_definitions;

		struct production
		{
			token_entry name;

			using rule = std::vector<token_entry>;

			std::vector<rule> rules;
		};

		std::vector<production> productions;
	};
}
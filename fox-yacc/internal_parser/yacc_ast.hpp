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
		std::vector<token_entry> c_declarations;

		// %start name
		token_entry start_identifier;

		// %variant type [type]
		std::vector<token_entry> variant_types;

		// %token [<tag>] name REGEX
		struct definition
		{
			token_entry rword;
			token_entry tag; // optional
			token_entry name;
			token_entry regex;
		};

		std::vector<definition> token_definitions;

		// %type <tag> name...
		std::unordered_map<token_entry, token_entry> name_type;

		struct production
		{
			token_entry name;

			using rule = std::vector<token_entry>;

			std::vector<rule> rules;
		};

		std::vector<production> productions;

		std::vector<std::string_view> c_definitions;
	};
}
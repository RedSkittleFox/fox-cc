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
		std::vector<std::string_view> c_declarations;

		// %start name
		token_entry* start_identifier;

		// %union { body of union (in C) }
		token_entry* union_identifier;

		// %token [<tag>] name [number] [name [number]]
		struct definition
		{
			token_entry* rword;
			token_entry* tag; // optional

			struct token
			{
				token_entry* name;
				int number;
			};

			std::vector<token> tokens;
		};

		std::vector<definition> token_definitions;

		// %type <tag> name...
		std::unordered_map<token_entry*, token_entry*> name_type;

		struct production
		{
			token_entry* name;

			struct rule
			{
				std::span<token_entry> tokens;
			};

			std::vector<rule> rules;
		};

		std::vector<production> productions;

		std::vector<std::string_view> c_definitions;
	};
}
#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace prs
{
	enum class token : std::uint32_t
	{
		IDENTIFIER = std::numeric_limits<char>::max() + 1, // char literals stand for themselves
		TAG, // < C_IDENTIFIER >
		C_STRING, 

		COLON,				// :
		SEMICOLON,			// ;
		COMMA,				// ,
		OR,					// |

		NUMBER,				// [0-9]+

		TYPE,				// %type
		LEFT,				// %left
		RIGHT,				// %right
		NONASSOC,			// %nonassoc
		TOKEN,				// %token
		PREC,				// %prec
		START,				// %start
		UNION,				// %union

		MARK,				// the %% mark
		LCURL,				// the %{ mark
		RCURL,				// the %} mark

		END_OF_FILE,		// end of file marker
		INVALID_TOKEN,		// invalid token marker
	};

	struct token_info
	{
		std::string_view token;
		uint32_t		line;
		uint32_t		column;

		std::string_view string_value;
		int				int_value;
	};

	struct token_entry // used inside a token list
	{
		token value;
		token_info* info;

		operator token() const { return value; }
	};

	inline token to_token(char c)
	{
		return static_cast<token>(c);
	}

	inline bool operator==(token lhs, char rhs)
	{
		return lhs == to_token(rhs);
	}

	inline bool operator==(char lhs, token rhs)
	{
		return rhs == lhs;
	}

	inline bool operator!=(token lhs, char rhs)
	{
		return lhs != to_token(rhs);
	}

	inline bool operator!=(char lhs, token rhs)
	{
		return rhs != lhs;
	}
}
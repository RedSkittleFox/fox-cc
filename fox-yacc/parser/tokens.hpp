#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace prs
{
	enum class token : std::uint32_t
	{
		LITERAL,
		IDENTIFIER,
		TAG, // < IDENTIFIER >

		C_DECLARATION,		// used for %{ C CODE %}
		C_ACTION,			// used for { C CODE }
		C_DEFINITION,		// used for the last %% segment

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

		END_OF_FILE,		// end of file marker
		INVALID_TOKEN,		// invalid token marker
		UNKNOWN,			// sane default value
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
		token value = token::UNKNOWN;
		token_info* info = nullptr;

		operator token() const { return value; }

		operator bool() const { return value != token::UNKNOWN; }
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

	/*
	 
	inline bool operator==(const token_entry& lhs, char rhs)
	{
		return lhs.value == to_token(rhs);
	}

	inline bool operator==(char lhs, const token_entry& rhs)
	{
		return rhs == lhs;
	}

	inline bool operator!=(const token_entry& lhs, char rhs)
	{
		return lhs.value != to_token(rhs);
	}

	inline bool operator!=(char lhs, const token_entry& rhs)
	{
		return rhs != lhs;
	}

	inline bool operator==(const token_entry& lhs, token rhs)
	{
		return lhs.value == rhs;
	}
	*/

	inline bool operator==(token lhs, const token_entry& rhs)
	{
		return rhs.value == lhs;
	}

	inline bool operator!=(const token_entry& lhs, token rhs)
	{
		return lhs.value != rhs;
	}

	inline bool operator!=(token lhs, const token_entry& rhs)
	{
		return rhs.value != lhs;
	}
}

template<>
struct std::hash<prs::token_entry>
{
	size_t operator()(const prs::token_entry& v) const noexcept
	{
		return
			std::hash<uint32_t>{}(std::to_underlying(v.value)) ^
			std::hash<void*>{}(v.info);
	}
};
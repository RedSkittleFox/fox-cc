#pragma once

#include <cstdint>
#include <limits>

namespace prs
{
	enum class token : std::uint32_t
	{
		IDENTIFIER = std::numeric_limits<char>::max() + 1, // char literals stand for themselves
		C_IDENTIFIER,		// identifier but not literal, followed by :
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
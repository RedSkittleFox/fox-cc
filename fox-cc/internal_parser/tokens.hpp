#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <sstream>

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
		REGEX,				// regex expression

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
		VARIANT,			// %variant

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

		[[nodiscard]] std::string to_string() const noexcept
		{
			std::stringstream token_string;
			switch (value) {
				using enum token;
#define ENUM_CASE(X) \
			case X : { \
				token_string << #X ; \
				break; \
			}
				ENUM_CASE(LITERAL);
				ENUM_CASE(IDENTIFIER);
				ENUM_CASE(TAG);
				ENUM_CASE(C_DECLARATION);
				ENUM_CASE(C_ACTION);
				ENUM_CASE(C_DEFINITION);
				ENUM_CASE(REGEX);
				ENUM_CASE(COLON);
				ENUM_CASE(SEMICOLON);
				ENUM_CASE(COMMA);
				ENUM_CASE(OR);
				ENUM_CASE(NUMBER);
				ENUM_CASE(TYPE);
				ENUM_CASE(LEFT);
				ENUM_CASE(RIGHT);
				ENUM_CASE(NONASSOC);
				ENUM_CASE(TOKEN);
				ENUM_CASE(PREC);
				ENUM_CASE(START);
				ENUM_CASE(VARIANT);
				ENUM_CASE(MARK);
				ENUM_CASE(END_OF_FILE);
				ENUM_CASE(INVALID_TOKEN);
				ENUM_CASE(UNKNOWN);
			}

			if (info)
				token_string << " : < "
				<< info->token << " , "
				<< info->line << " , "
				<< info->column << " >"
				;

			return token_string.str();
		}
	};

	inline token to_token(char c)
	{
		return static_cast<token>(c);
	}

	/*
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
	*/

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

	/*
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
	*/
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
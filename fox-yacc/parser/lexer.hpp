#pragma once

#include <string_view>
#include <unordered_map>

#include "tokens.hpp"

namespace prs
{
	class lexer
	{
		std::string_view input_;
		std::unordered_map<std::string_view, token_info> tokens_;
		size_t current_ = 0;
		char c_;

		size_t debug_line_ = 0;
		size_t debug_col_ = 0;

		size_t pushed_current_;
		size_t pushed_debug_line_;
		size_t pushed_debug_col_;

	public:
		lexer() = delete;

		lexer(std::string_view input)
			: input_(input)
		{
			c_ = input_[current_];
		}

	public:
		token_entry next_token();

	private:
		void marker_forward();
		void marker_backward();

		void assert_marker_in_range();

		char c() const;
		bool in_range() const;

		void push_state();
		bool pop_state();

		bool build_token_entry(token type, token_entry& entry, std::string_view sv = {}, int v = {});

	private:
		// consumes all white-spaces
		bool consume_space();

		// consumes /**/ and // comments
		bool consume_comment();

		// [0-9]+
		bool interpret_number(token_entry& entry);

		// [a-zA-Z][a-zA-Z_0-9]*
		bool interpret_identifier(token_entry& entry);

		// 'CHAR' or 'ESCAPE'
		bool interpret_literal(token_entry& entry);

		// < C_IDENTIFIER >
		bool interpret_tag(token_entry& entry);

		// { ... anything ... }
		bool interpret_c_string(token_entry& entry);

		// [:]
		bool interpret_colon(token_entry& entry);

		// [;]
		bool interpret_semicolon(token_entry& entry);

		// [,]
		bool interpret_comma(token_entry& entry);

		// [|]
		bool interpret_or(token_entry& entry);

		// %[command]
		bool interpret_command(token_entry& entry);

		// generate invalid
		bool generate_invalid_token(token_entry& entry);
	};
}
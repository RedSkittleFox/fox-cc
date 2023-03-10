#pragma once

#include <string_view>
#include <unordered_map>
#include <stdexcept>
#include <deque>

#include "tokens.hpp"

namespace prs
{
	class lexer
	{
		std::string_view input_;
		std::deque<token_info> tokens_; // we don't want reallocations here!
		size_t current_ = 0;
		char c_;

		size_t debug_line_ = {};
		size_t debug_col_ = {};

		size_t pushed_current_ = {};
		size_t pushed_debug_line_ = {};
		size_t pushed_debug_col_ = {};

	public:
		lexer() = delete;

		lexer(std::string_view input)
			: input_(input)
		{
			if (std::empty(input_))
				throw std::logic_error("Input file is empty.");

			c_ = input_[current_];
		}

		lexer(const lexer&) = delete;

		lexer(lexer&&) noexcept = default;

		lexer& operator=(const lexer&) = delete;

		lexer& operator=(lexer&&) noexcept = default;

		~lexer() noexcept = default;

	public:
		token_entry next_token();
		token_entry c_definition_token();

	private:
		// Move marker forward
		void marker_forward();

		// Assert if marker is in range
		void assert_marker_in_range();

		// Returns current character
		[[nodiscard]] char c() const;

		// Returns if marker is in range
		[[nodiscard]] bool in_range() const;

		// Saves current lexer state
		void push_state();

		// Restores saved lexer state
		bool pop_state();

		// Creates entry for the token
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
		bool interpret_c_action(token_entry& entry);

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
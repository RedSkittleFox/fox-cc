#pragma once

#include <cassert>
#include <variant>
#include <string_view>
#include <vector>
#include <type_traits>

#include <regex_compiler/regex_compiler_exception.hpp>
#include <automata/nfa.hpp>
#include <fox_cc.hpp>

namespace fox_cc
{
	namespace regex_compiler
	{
		// converts regex to reverse polish notation regular expression, resolves character classes
		class regex_parser
		{
			enum class regex_parser_op
			{
				closure_zero_more,
				closure_one_more,
				closure_zero_one,
				concatenation,
				alternative
			};

			[[nodiscard]] static size_t regex_parser_op_priority(regex_parser_op op);

			using regex_parser_token_charset = charset;
			using regex_parser_token_end = std::integral_constant<size_t, 0>;
			using regex_parser_token_left_parenthesis = std::integral_constant<size_t, 1>;
			using regex_parser_token_right_parenthesis = std::integral_constant<size_t, 2>;

			using regex_parser_token = std::variant
			<
				std::monostate,
				regex_parser_token_charset,
				regex_parser_op,
				regex_parser_token_left_parenthesis,
				regex_parser_token_right_parenthesis,
				regex_parser_token_end
			>;

			std::string_view str_;
			size_t lexer_current_char_pos_ = {};
			char lexer_current_char_ = std::empty(str_) ? '\0' : str_[lexer_current_char_pos_];

			regex_parser_token c0_ = {};
			regex_parser_token c1_ = {};

		private:
			[[noreturn]] void throw_error(const char* error_string) const noexcept(false);

		public:
			regex_parser() = delete;
			regex_parser(const regex_parser&) = delete;
			regex_parser(regex_parser&&) noexcept = delete;
			regex_parser& operator=(const regex_parser&) = delete;
			regex_parser& operator=(regex_parser&&) noexcept = delete;

		public:
			regex_parser(std::string_view str) noexcept
				: str_(str) {}

			~regex_parser() noexcept = default;

		private:
			[[nodiscard]] regex_parser_token token();

			[[nodiscard]] regex_parser_token lexer_token();
			[[nodiscard]] bool lexer_escape_char(regex_parser_token& t);
			[[nodiscard]] bool lexer_char_group(regex_parser_token& t);
			[[nodiscard]] bool lexer_char(regex_parser_token& t);
			[[nodiscard]] bool lexer_operator(regex_parser_token& t);
			[[nodiscard]] bool lexer_end(regex_parser_token& t);
			[[nodiscard]] bool lexer_error(regex_parser_token& t);

			[[nodiscard]] char lexer_char() const noexcept;
			void lexer_next_char() noexcept;

		private:
			// converts to reverse polish notation!
			std::vector<regex_parser_token> compile_rpn();

		private:
			using nfa = fox_cc::automata::nfa<automata::empty_state, size_t, charset>;

			[[nodiscard]] static nfa nfa_empty_expression();
			[[nodiscard]] static nfa nfa_charset_expression(charset ch);
			[[nodiscard]] static nfa nfa_concatenation_expression(const nfa& lhs, const nfa& rhs);
			[[nodiscard]] static nfa nfa_union_expression(const nfa& lhs, const nfa& rhs);
			[[nodiscard]] static nfa nfa_closure_one_more_expression(const nfa& expr);
			[[nodiscard]] static nfa nfa_closure_zero_one_expression(const nfa& expr);
			[[nodiscard]] static nfa nfa_closure_zero_more_expression(const nfa& expr);

			nfa compile_nfa(const std::vector<regex_parser_token>& rpn);

		public:
			[[nodiscard]] nfa compile();
		};
	}
}
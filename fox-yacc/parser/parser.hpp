#pragma once

#include <optional>
#include <type_traits>
#include <cassert>

#include "lexer.hpp"
#include "yacc_ast.hpp"

namespace prs
{
	class parser
	{
		lexer& lexer_;
		yacc_ast ast_;

		token_entry e0_;
		token_entry e1_;

	public:
		parser() = delete;

		parser(lexer& lexer)
			: lexer_(lexer)
		{
			e0_ = lexer_.next_token();
			e1_ = lexer_.next_token();
		}

		parser(const parser&) = delete;

		parser(parser&&) noexcept = default;

		parser& operator=(const parser&) = delete;

		parser& operator=(parser&&) noexcept = default;

		~parser() noexcept = default;

	private:
		void next_token();
		void next_regex_token();

		[[nodiscard]] token_entry e0() const noexcept;
		[[nodiscard]] token_entry e1() const noexcept;

	public:
		[[nodiscard]] yacc_ast ast() const noexcept;

		void parse();

	private:
		// spec : definitions MARK rules tail
		void parse_spec();
		void parse_defs();
		void parse_prods();
		void parse_tail();

		void parse_def();
		void parse_def_start();
		void parse_def_variant();
		void parse_def_token();
		void parse_def_type();
		void parse_def_c_decl();

		void parse_prod();

	private:
		void error(
			const char* msg = nullptr, 
			const char* solution = nullptr
		)
		{
			assert(false);
		}

		void expect(token t)
		{
			assert(t == e0());
		}

		template<class... Args>
		requires std::conjunction_v<std::is_same<Args, token>...>
		void expect(Args... t)
		{
			token u = e0();
			bool v = ( (t == u) || ... );
			assert(v);
		}
	};
}
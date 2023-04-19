#pragma once

#include <fox_cc.hpp>
#include <internal_parser/yacc_ast.hpp>

#include <bitset>
#include <map>
#include <automata/dfa.hpp>

namespace fox_cc
{
	class lex_compiler
	{
	public:
		using charset = std::bitset<128>;

		struct lex_compiler_result
		{
			enum class associativity
			{
				token,
				left,
				right,
				nonassoc
			};

			struct lex_token
			{
				std::string name;
				std::string type;
				associativity assoc;
			};

			std::vector<lex_token> terminals; // maps IDs to terminals
			automata::dfa<automata::empty_state, size_t, charset> dfa;
		};

	private:
		const prs::yacc_ast& ast_;
		lex_compiler_result result_;

	public:
		lex_compiler() = delete;
		lex_compiler(const lex_compiler&) = delete;
		lex_compiler(lex_compiler&&) noexcept = delete;
		lex_compiler& operator=(const lex_compiler&) = delete;
		lex_compiler& operator=(lex_compiler&&) noexcept = delete;

	public:
		lex_compiler(const prs::yacc_ast& ast);

		~lex_compiler() noexcept = default;

		[[nodiscard]] const lex_compiler_result& result() const noexcept
		{
			return result_;
		}

	private:
		void error(const char* what) {};
		void warning(const char* what) {};
	};
}
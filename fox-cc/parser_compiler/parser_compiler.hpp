#pragma once

#include <variant>
#include <vector>
#include <limits>

#include <internal_parser/yacc_ast.hpp>
#include <lex_compiler/lex_compiler.hpp>
#include <automata/dfa.hpp>

namespace fox_cc
{
	class parser_compiler
	{
	public:
		struct parser_compiler_result
		{
			using token_id = size_t;
			static inline constexpr token_id token_id_npos = std::numeric_limits<token_id>::max();
			static inline constexpr token_id end_token = static_cast<token_id>(0);

			struct terminal
			{
				token_id id;
				std::string name;
				lex_compiler::lex_compiler_result::associativity assoc;
			};

			struct non_terminal
			{
				token_id id;
				std::string name;
				std::vector<std::vector<size_t>> productions;
			};

			struct token : std::variant<terminal, non_terminal>
			{
				token(const terminal& v)
					: variant(v) {}

				token(const non_terminal& v)
					: variant(v) {}

				[[nodiscard]] bool is_terminal() const noexcept
				{
					return std::holds_alternative<struct terminal>(*this);
				}

				[[nodiscard]] bool is_non_terminal() const noexcept
				{
					return std::holds_alternative<struct non_terminal>(*this);
				}

				[[nodiscard]] struct terminal& terminal() noexcept
				{
					return std::get<struct terminal>(*this);
				}

				[[nodiscard]] const struct terminal& terminal() const noexcept
				{
					return std::get<struct terminal>(*this);
				}

				[[nodiscard]] struct non_terminal& non_terminal() noexcept
				{
					return std::get<struct non_terminal>(*this);
				}

				[[nodiscard]] const struct non_terminal& non_terminal() const noexcept
				{
					return std::get<struct non_terminal>(*this);
				}

				[[nodiscard]] const std::string& name() const noexcept
				{
					return std::visit([](const auto& v) -> const std::string& {return v.name; }, *this);
				}

				[[nodiscard]] token_id id() const noexcept
				{
					return std::visit([](const auto& v) {return v.id; }, *this);
				}
			};

			struct state_data
			{
				struct production
				{
					token_id non_terminal;
					token_id non_terminal_production;
					size_t current;

					std::set<token_id> follow_set;

					friend bool operator==(const production& lhs, const production& rhs)
					{
						return
							lhs.non_terminal == rhs.non_terminal &&
							lhs.non_terminal_production == rhs.non_terminal_production &&
							lhs.current == rhs.current &&
							lhs.follow_set == rhs.follow_set;
					}
				};

				std::vector<production> productions;
				
				friend bool operator==(const state_data& lhs, const state_data& rhs)
				{
					return lhs.productions == rhs.productions;
				}

				enum class action
				{
					shift,
					reduce,
					accept
				};

				struct action_t
				{
					action action;
					union
					{
						size_t state;
						
					};
				};

				struct action_accept {};

				struct action_reduce
				{
					size_t production_id;
					size_t pop_count;
					size_t push_state;
				};

				struct action_shift
				{
					size_t production_id;
					size_t goto_state;
				};

				std::unordered_map<size_t, std::variant<action_accept, action_reduce, action_shift>> action_table;
			};

			std::vector<token> tokens;
			automata::dfa<state_data, size_t, size_t> dfa;
		};

	private:
		const lex_compiler::lex_compiler_result& lex_result_;
		const prs::yacc_ast& ast_;
		parser_compiler_result result_;

		parser_compiler_result::token_id first_non_terminal_;
		std::vector<std::set<parser_compiler_result::token_id>> first_sets_;

	public:
		parser_compiler() = delete;
		parser_compiler(const parser_compiler&) = delete;
		parser_compiler(parser_compiler&&) noexcept = delete;
		parser_compiler& operator=(const parser_compiler&) = delete;
		parser_compiler& operator=(parser_compiler&&) noexcept = delete;

	public:
		parser_compiler(const lex_compiler::lex_compiler_result& lex_result, const prs::yacc_ast& ast);
		~parser_compiler() = default;

	public:
		[[nodiscard]] const parser_compiler_result& result() const noexcept
		{
			return result_;
		}

	private:
		void init_terminals();
		void init_non_terminals();
		void generate_first_sets();
		void init_first_state();
		void init_states();

		void populate_state(parser_compiler_result::state_data& state);
		bool insert_production(parser_compiler_result::state_data& state, size_t production_id, std::set<size_t> follow_set);

		void compute_actions();

	private:
		[[nodiscard]] parser_compiler_result::token_id token_by_name(const std::string& name) const noexcept;
		[[nodiscard]] const std::set<parser_compiler_result::token_id>& first_set(parser_compiler_result::token_id id) const noexcept;
		[[nodiscard]] std::set<parser_compiler_result::token_id>& first_set(parser_compiler_result::token_id id) noexcept;
		void error(const char* msg);

	private:
		bool reduce_reduce_conflict(size_t node, size_t lhs_prod, size_t rhs_prod);
		bool shift_reduce_conflict(size_t node, size_t lhs_prod, size_t rhs_prod);
	};
}

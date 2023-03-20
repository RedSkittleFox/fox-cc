#pragma once

#include <vector>
#include <set>
#include <map>
#include <numeric>
#include <algorithm>
#include <map>
#include <span>
#include <iostream>
#include <ranges>

#include "../parser/parser.hpp"
#include "compiler_types.hpp"

namespace cmp
{
	class compiler
	{
		static constexpr auto default_type = "int";

		const prs::yacc_ast& ast_;

		// List of types
		std::set<std::string> types_;

		enum class token_assoc
		{
			first,
			left,
			right,
			no_assoc
		};

		struct token
		{
			const std::string* type = nullptr;	// use to see if token is valid
			token_id_t token_id;				// token id
			std::string name;					// token name/value
			token_assoc assoc;					// associativity
			bool terminal;						// is a terminal symbol

			// Debug information
			prs::token_entry prs_token_definition;
		};

		// List of tokens, position corresponds to token id
		std::vector<token> tokens_;

		// Pointer to default token type (if unspecified)
		const std::string* p_default_token_type_;

		// Token free list, used during token list generation
		std::vector<uint16_t> token_free_list_;

		// Starting production in the grammar
		non_terminal_id_t start_production_;

		struct action
		{
			action(prs::token_entry)
			{
				
			}
		};

		std::vector<action> actions_;

		// Map of non-terminals and productions
		std::multimap<non_terminal_id_t, std::vector<production_symbol_t>> productions_;

		// First sets (sets of terminals that a non-terminal can start with)
		std::map<non_terminal_id_t, std::set<terminal_id_t>> first_sets_;

		// Structure describing the state in a LR(1) parser
		struct state
		{
			struct production
			{
				non_terminal_id_t name;
				std::span<const production_symbol_t> stack;
				std::span<const production_symbol_t> prod;

				auto operator<=>(const production& other) const noexcept
				{
					if (const auto cmp = name <=> other.name; cmp != nullptr)
						return cmp;

					if (const auto cmp =
						std::lexicographical_compare_three_way
						(
							std::begin(stack), std::end(stack),
							std::begin(other.stack), std::end(other.stack));
							cmp != nullptr
						)
						return cmp;

					return std::lexicographical_compare_three_way
					(
						std::begin(prod), std::end(prod),
						std::begin(other.prod), std::end(other.prod)
					);
				}

				friend bool operator==(const production& lhs, const production& rhs) 
				{
					return
						lhs.name == rhs.name &&
						std::ranges::equal(lhs.stack, rhs.stack) &&
						std::ranges::equal(lhs.prod, rhs.prod);

				}
			};

			std::map<production, std::set<terminal_id_t>> productions;

			std::map<token_id_t /*token in*/, size_t /*state id*/> transitions; // Transitions 
		};

		std::vector<state> states_;

	public:
		compiler() = delete;

		compiler(const prs::yacc_ast& ast)
			: ast_(ast), tokens_(512) {}

		compiler(const compiler&) = delete;

		compiler(compiler&&) noexcept = delete;

		compiler& operator=(const compiler&) = delete;
		compiler& operator=(compiler&&) noexcept = delete;

		~compiler() noexcept = default;

	public:
		void compile()
		{
			// Create default type 
			p_default_token_type_ = std::addressof(*types_.insert(default_type).first);

			token_free_list_ = std::vector<uint16_t>(512 - 128);
			std::iota(std::rbegin(token_free_list_), std::rend(token_free_list_), 128); // generate a free list

			// Generate tokens table
			generate_char_tokens();
			generate_terminal_tokens();
			generate_production_tokens();
			optimize_token_table();

			// Productions
			generate_production_table();

			generate_first_sets();
			generate_states();
		}

	private:
		void generate_char_tokens();
		void generate_terminal_tokens();
		void generate_production_tokens();
		void optimize_token_table();

	private:
		void generate_production_table();
		void generate_first_sets();
		void generate_states();

		[[maybe_unused]] bool closure_populate_state(size_t state_id);
		[[maybe_unused]] bool closure_generate_follow_sets(size_t state_id);
		[[maybe_unused]] std::vector<size_t> generate_gotos(size_t state_id);
		void merge_states(size_t source_state, std::vector<size_t>& new_states);

		bool state_insert_productions(state& s, non_terminal_id_t production);

	private:
		token* token_by_name(std::string_view sv);
		token_id_t token_id_by_name(std::string_view sv);


		[[noreturn]] void error(const char* msg)
		{
			assert(false && msg);
		}

		void assert_unique_name(prs::token_entry e)
		{
			auto r = std::ranges::find_if(tokens_,
				[=](const token& t)->bool {return t.name == e.info->string_value; });

			assert(r == std::end(tokens_));
		}

	public:
		void debug_print();

	};
}

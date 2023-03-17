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

namespace cmp
{
	class compiler
	{
		static constexpr auto default_type = "int";

		const prs::yacc_ast& ast_;

		std::set<std::string, std::less<>> types_;

		struct token
		{
			const std::string* type = nullptr; // use to see if token is valid
			size_t value;
			std::string name;
			// TODO: Assoc
			bool terminal;
		};

		std::vector<token> tokens_;

		const std::string* p_default_t_;
		std::vector<uint16_t> token_free_list_;

		size_t start_production_;
		std::multimap<size_t, std::vector<size_t>> productions_;
		std::map<size_t, std::set<size_t>> first_sets_;

		struct state
		{
			struct production
			{
				size_t name;
				std::span<const size_t> stack;
				std::span<const size_t> prod;

				mutable std::set<size_t> follow; // HACK: this does not affect comparison

				std::strong_ordering operator<=>(const production& prod) const
				{
					if (auto cmp = name <=> prod.name; cmp != 0)
						return cmp;

					if (auto cmp =
						std::lexicographical_compare_three_way(std::begin(stack), std::end(stack),
							std::begin(prod.stack), std::end(prod.stack)); cmp != 0)
						return cmp;

					if (auto cmp =
						std::lexicographical_compare_three_way(std::begin(this->prod), std::end(this->prod),
							std::begin(prod.prod), std::end(prod.prod)); cmp != 0)
						return cmp;

					return std::strong_ordering(0); // don't distinguish by follow set

					// return std::lexicographical_compare_three_way(std::begin(follow), std::end(follow),
					// 	std::begin(prod.follow), std::end(prod.follow));
				}
			};

			std::set<production> productions;
			std::map<size_t, size_t> transitions;
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
			p_default_t_ = std::addressof(*types_.insert(default_type).first);

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

		bool state_insert_productions(state& s, size_t production, const std::set<size_t>& followers = {});

	private:
		token* token_by_name(std::string_view sv);
		size_t token_id_by_name(std::string_view sv);


		[[noreturn]] void error(const char* msg)
		{
			assert(false && msg);
		}

		void assert_unique_name(prs::token_entry e)
		{
			auto r = std::find_if(std::begin(tokens_), std::end(tokens_),
				[=](const token& t)->bool {return t.name == e.info->string_value; });

			assert(r == std::end(tokens_));
		}

	public:
		void debug_print();

	};
}

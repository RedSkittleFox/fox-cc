#pragma once

#include <vector>
#include <string>
#include <cassert>
#include <bitset>

#include <internal_parser/lexer.hpp>
#include <internal_parser/parser.hpp>
#include <lex_compiler/lex_compiler.hpp>
#include <parser_compiler/parser_compiler.hpp>

namespace fox_cc
{
	using charset = std::bitset<128>;

	template<class T>
	class fixed_2D_array
	{
		size_t width_;
		size_t height_;

		T* data_;

	public:
		void clear() noexcept
		{
			delete[] data_;
			width_ = 0;
			height_ = 0;
		}

	public:
		fixed_2D_array() : width_(0), height_(0), data_(nullptr) {}

		fixed_2D_array(size_t width, size_t height, T default_val = T())
			: width_(width), height_(height), data_(new T[width * height])
		{
			for (auto s = std::ranges::subrange(data_, data_ + width * height); auto & i : s)
				i = default_val;
		}

		fixed_2D_array(const fixed_2D_array& other) = delete;
		fixed_2D_array& operator=(const fixed_2D_array& other) = delete;

	public:
		fixed_2D_array(fixed_2D_array&& other) noexcept
			:
			width_(std::exchange(other.width_, {})),
			height_(std::exchange(other.height_, {})),
			data_(std::exchange(other.data_, {}))
		{}

		fixed_2D_array& operator=(fixed_2D_array&& other) noexcept
		{
			this->clear();

			width_ = std::exchange(other.width_, {});
			height_ = std::exchange(other.height_, {});
			data_ = std::exchange(other.data_, {});
			return *this;
		}

		~fixed_2D_array() noexcept
		{
			this->clear();
		}

		T* operator[](size_t idx) noexcept
		{
			assert(idx * height_ < width_ * height_);

			return std::addressof(data_[idx * height_]);
		}

		const T* operator[](size_t idx) const noexcept
		{
			assert(idx * height_ < width_ * height_);

			return std::addressof(data_[idx * height_]);
		}
	};

	class ast
	{
		class ast_node
		{
			union child_t
			{
				ast_node* ptr;
				size_t offset;
			};

			std::string_view type_;
			std::string_view value_;
			std::vector<child_t> children_;
		public:

		};

		std::vector<std::string> vector_;
		std::vector<ast_node> ast_nodes_;
	};

	class compiler
	{
		lex_compiler::lex_compiler_result lexer_;
		parser_compiler::parser_compiler_result parser_;

	public:
		compiler() = delete;

		compiler(std::string_view language)
		{
			this->assign(language);
		}

		compiler(const compiler&) = default;

		compiler(compiler&&) noexcept = default;

		compiler& operator=(const compiler&) = default;

		compiler& operator=(compiler&&) noexcept = default;

		~compiler() noexcept = default;

	public:
		const lex_compiler::lex_compiler_result& lexer() const noexcept
		{
			return lexer_;
		}

		const parser_compiler::parser_compiler_result& parser() const noexcept
		{
			return parser_;
		}

	public:
		void assign(std::string_view language)
		{
			prs::lexer lx(language);
			prs::parser ps(lx);
			ps.parse();
			auto ast = ps.ast();

			fox_cc::lex_compiler lex_cmp(ast);
			lexer_ = lex_cmp.result();
			fox_cc::parser_compiler prs_cmp(lexer_, ast);
			parser_ = prs_cmp.result();

			auto& result = parser_;

			for (size_t i = 0; i < result.dfa.size(); ++i)
			{
				auto& state = result.dfa[i];

				std::cout << "===== " << i << "=====\n";

				for (auto prod : state.value().productions)
				{
					auto& nt = result.tokens[prod.non_terminal].non_terminal();

					std::cout << nt.name << " -> ";

					auto& sub_prod = nt.productions[prod.non_terminal_production];

					for (size_t i = 0; i <= sub_prod.size(); ++i)
					{
						if (i == prod.current)
							std::cout << ". ";

						if (i < sub_prod.size())
							std::cout << result.tokens[sub_prod[i]].name() << " ";
					}

					std::cout << " [ ";
					for (auto follow : prod.follow_set)
						std::cout << result.tokens[follow].name() << ' ';
					std::cout << "]\n";

				}

				std::cout << "\n";

				for (auto go_to : state.next())
				{
					std::cout << result.tokens[go_to.first].name() << " : " << go_to.second << '\n';
				}

				std::cout << "\n";
			}
		}

	public:
		[[nodiscard]] ast compile(std::string_view input) const
		{
			// TODO: Else
			assert(std::size(input) >= 2);

			
			char c0 = 0, c1 = input[0];

			auto& lexer = lexer_.dfa;
			auto& parser = parser_.dfa;

			size_t current_lexer_node = lexer.start();

			size_t potential_reduce_i = std::numeric_limits<size_t>::max();
			size_t potential_reduce_node = current_lexer_node;

			size_t token_start = 0;

			auto lexer_function = [&, lexer_i = static_cast<size_t>(0)](size_t& out_start, size_t& out_end) mutable -> size_t
			{
				for (; lexer_i < std::size(input); ++lexer_i)
				{
					if (lexer_i == std::size(input))
					{
						out_start = out_end = std::size(input);
						return 0;
					}

					c0 = input[lexer_i];
					c1 = (lexer_i + 1 < std::size(input)) ? input[lexer_i + 1] : 0;

					auto& node = lexer[current_lexer_node];

					if (node.reduce())
					{
						if (potential_reduce_i == lexer_i) // Reduce
						{
							out_start = token_start;
							out_end = potential_reduce_i;
							token_start = potential_reduce_i + 1;
							potential_reduce_i = std::numeric_limits<size_t>::max();
							current_lexer_node = lexer.start();
							// lexer_i -= 1; // redo current char
							// lexer_i += 1;
							return node.reduce().value();
						}
						else // Try matching longer string
						{
							potential_reduce_i = lexer_i;
							potential_reduce_node = current_lexer_node;
						}
					}

					bool matched = false;
					for (auto& n = node.next(); auto & c : n)
					{
						if (c.first.test(c0))
						{
							matched = true;
							current_lexer_node = c.second;
							break;
						}
					}

					if (matched == false)
					{
						if (potential_reduce_i == std::numeric_limits<size_t>::max())
						{
							throw "Unknown token\n";
						}
						else
						{
							lexer_i = potential_reduce_i - 1;
							current_lexer_node = potential_reduce_node;
						}
					}
				}

				if (potential_reduce_i != std::numeric_limits<size_t>::max())
				{
					if(potential_reduce_i != std::size(input) - 1)
					{
						throw "Error";
					}
					else
					{
						out_start = token_start;
						out_end = potential_reduce_i;
						potential_reduce_i = std::numeric_limits<size_t>::max();
						return lexer[potential_reduce_node].reduce().value();
					}
				}

				return {};
			};

			size_t start, end;

			std::vector<size_t> reduction_stack;
			//reduction_stack.push_back(parser.start());
			reduction_stack.push_back(0);

			size_t e0 = lexer_function(start, end), e1 = lexer_function(start, end);
			auto next_token = [&]() {e0 = e1; e1 = lexer_function(start, end); };

			while(true)
			{
				size_t state_id = reduction_stack.back();

				auto look_ahead = e0;

				auto r = parser[state_id].value().action_table.find(look_ahead);
				if (r == std::end(parser[state_id].value().action_table))
				{
					throw "Compilation error at token...";
				}

				if (std::holds_alternative<parser_compiler::parser_compiler_result::state_data::action_shift>(r->second))
				{
					const auto& action = std::get<parser_compiler::parser_compiler_result::state_data::action_shift>(r->second);

					auto shifted_token = e0;
					next_token();
					reduction_stack.push_back(shifted_token);
					auto new_state = parser[state_id].next().at(shifted_token);
					reduction_stack.push_back(new_state);
				}
				else if (std::holds_alternative<parser_compiler::parser_compiler_result::state_data::action_reduce>(r->second))
				{
					const auto& action = std::get<parser_compiler::parser_compiler_result::state_data::action_reduce>(r->second);
					for (size_t i = 0; i < action.pop_count + 1; ++i)
						reduction_stack.pop_back();
					state_id = reduction_stack.back();
					auto n = action.push_state;
					reduction_stack.push_back(n);
					auto new_state = parser[state_id].next().at(n);
					reduction_stack.push_back(new_state);
				}
				else
				{
					assert(false);
				}

				std::cout << lexer_.terminals[e0].name << '\n';
			}

			auto match_token = [&, this, e0 = std::numeric_limits<size_t>::max()](size_t e1, size_t start, size_t end) mutable
			{
				
			};


			return {};
		}
	};
}
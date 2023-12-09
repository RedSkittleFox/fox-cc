#pragma once

#include <vector>
#include <string>
#include <cassert>
#include <bitset>
#include <functional>
#include <sstream>

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

	struct ast
	{
		struct terminal
		{
			std::string type;
			std::string value;
		};

		class ast_node
		{
			std::vector<std::variant<ast_node, terminal>> children;
		};

		ast_node root;
	};

	class compiler
	{
		lex_compiler::lex_compiler_result lexer_;
		parser_compiler::parser_compiler_result parser_;

		std::unordered_map<std::string, std::function<std::string(std::span<std::string>)>> actions_;

	public:
		void register_action(const std::string& name, const std::function<std::string(std::span<std::string>)>& func)
		{
			actions_[name] = func;
		}

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
		}

	public:
		[[nodiscard]] std::string parser_to_string() const
		{
			std::stringstream ss;

			auto& result = parser_;

			for (size_t i = 0; i < result.dfa.size(); ++i)
			{
				auto& state = result.dfa[i];

				ss << "===== " << i << "=====\n";

				for (auto prod : state.value().productions)
				{
					auto& nt = result.tokens[prod.non_terminal].non_terminal();

					ss << nt.name << " -> ";

					auto& sub_prod = nt.productions[prod.non_terminal_production];

					for (size_t i = 0; i <= sub_prod.size(); ++i)
					{
						if (i == prod.current)
							ss << ". ";

						if (i < sub_prod.size())
							ss << result.tokens[sub_prod[i]].name() << " ";
					}

					ss << " [ ";
					for (auto follow : prod.follow_set)
						ss << result.tokens[follow].name() << ' ';
					ss << "]\n";

				}

				ss << "\n";

				for (auto go_to : state.next())
				{
					ss << result.tokens[go_to.first].name() << " : " << go_to.second << '\n';
				}

				ss << "\n";
			}

			return ss.str();
		}

		[[nodiscard]] std::string lexer_to_string() const
		{
			std::stringstream ss;

			auto& out = lexer_.dfa;

			ss << "============\n";

			for (size_t i = 0; i < out.size(); ++i)
			{
				ss << i << '\n';

				for (auto to : out[i].next())
				{
					if constexpr (std::is_same_v<const std::bitset<128>, decltype(to.first)>)
					{
						ss << "\t" << to.second << " | ";
						if (to.first.all())
						{
							ss << "EPSILON";
						}
						else
						{
							for (size_t i = 0; i < 128; ++i)
							{
								if (to.first.test(i))
								{
									ss << static_cast<char>(i) << ' ';
								}
							}
						}

						ss << '\n';
					}
					else
					{
						ss << "\t" << to.second << " | " << to.first << '\n';
					}
				}
			}

			return ss.str();
		}

		[[nodiscard]] std::string dot_to_string() const 
		{
			std::stringstream ss;

			auto& cmp = *this;
			auto& result = cmp.parser();
			ss << "digraph G {\n";
			for (size_t i = 0; i < result.dfa.size(); ++i)
			{
				auto& state = result.dfa[i];
				ss << i << " [ label = \"";
				ss << i << "\n";
				for (auto prod : state.value().productions)
				{
					auto& nt = result.tokens[prod.non_terminal].non_terminal();

					ss << nt.name << " -> ";

					auto& sub_prod = nt.productions[prod.non_terminal_production];

					for (size_t i = 0; i <= sub_prod.size(); ++i)
					{
						if (i == prod.current)
							ss << ". ";

						if (i < sub_prod.size())
							ss << result.tokens[sub_prod[i]].name() << " ";
					}

					ss << " ( ";
					for (auto follow : prod.follow_set)
						ss << result.tokens[follow].name() << ' ';
					ss << ")\n";

				}
				ss << "\"];\n";

				for (auto go_to : state.next())
				{
					ss << i << " -> " << go_to.second << "[label=\"" << result.tokens[go_to.first].name() << " : " << go_to.first << "\"];\n";
				}

				ss << "\n";
			}

			ss << "}\n";

			return ss.str();
		}

	public:
		[[nodiscard]] std::string compile(std::string_view input) const
		{
			std::stringstream ss;
			return compile(input, ss);
		}

		[[nodiscard]] std::string compile(std::string_view input, std::ostream& os) const
		{
			// TODO: Else
			assert(std::size(input) >= 2);

			ast out;

			char c0 = 0, c1 = input[0];

			auto& lexer = lexer_.dfa;
			auto& parser = parser_.dfa;

			size_t current_lexer_node = lexer.start();

			size_t potential_reduce_i = std::numeric_limits<size_t>::max();
			size_t potential_reduce_node = current_lexer_node;

			size_t token_start = 0;

			std::vector<ast::terminal> lexer_tokens;

			os << "node [symbol] node [symbol] ...\n";

			auto lexer_function = [&, lexer_i = static_cast<size_t>(0)](size_t& out_start, size_t& out_end) mutable -> size_t
			{
				std::size_t ts = lexer_i;
				for (; lexer_i <= std::size(input); ++lexer_i)
				{
					c0 = (lexer_i < std::size(input)) ? input[lexer_i] : 0;
					c1 = (lexer_i + 1 < std::size(input)) ? input[lexer_i + 1] : 0;

					auto& node = lexer[current_lexer_node];

					if (node.reduce())
					{
						if (potential_reduce_i == lexer_i) // Reduce
						{
							lexer_tokens.push_back(
								ast::terminal{
								lexer_.terminals[node.reduce().value()].name,
								static_cast<std::string>(input.substr(ts, 
								lexer_i - ts))
								});

							out_start = token_start;
							out_end = potential_reduce_i;
							token_start = potential_reduce_i + 1;
							potential_reduce_i = std::numeric_limits<size_t>::max();
							current_lexer_node = lexer.start();
							return node.reduce().value();
						}
						else // Try matching longer string
						{
							potential_reduce_i = lexer_i;
							potential_reduce_node = current_lexer_node;
						}
					}

					if (lexer_i == std::size(input) && potential_reduce_i == std::numeric_limits<size_t>::max())
					{
						out_start = out_end = std::size(input);
						lexer_tokens.push_back(
							ast::terminal{lexer_.terminals[0].name }
						);
						return 0;
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
						assert(false); // This should be a dead path
						return lexer[potential_reduce_node].reduce().value();
					}
				}

				lexer_tokens.push_back(
					ast::terminal{ lexer_.terminals[0].name }
				);
				return {};
			};

			size_t start, end;

			std::vector<size_t> reduction_stack;
			std::vector<std::string> value_stack;
			//reduction_stack.push_back(parser.start());
			reduction_stack.push_back(0);

			size_t e0 = lexer_function(start, end);
			size_t e1 = lexer_function(start, end);
			auto next_token = [&]() {e0 = e1; e1 = lexer_function(start, end); };

			bool modified = true;
			while(modified)
			{
				modified = false;
				size_t state_id = reduction_stack.back();

				auto r = parser[state_id].value().action_table.find(e0);
				if (r == std::end(parser[state_id].value().action_table))
				{
					throw std::logic_error("Compilation error at token...");
				}

				if (std::holds_alternative<parser_compiler::parser_compiler_result::state_data::action_shift>(r->second))
				{
					const auto& action = std::get<parser_compiler::parser_compiler_result::state_data::action_shift>(r->second);

					auto shifted_token = e0;
					value_stack.push_back(lexer_tokens[std::size(lexer_tokens) - 2].value);
					next_token();
					reduction_stack.push_back(shifted_token);
					auto new_state = parser[state_id].next().at(shifted_token);
					reduction_stack.push_back(new_state);
					modified = true;
				}
				else if (std::holds_alternative<parser_compiler::parser_compiler_result::state_data::action_reduce>(r->second))
				{
					const auto& action = std::get<parser_compiler::parser_compiler_result::state_data::action_reduce>(r->second);

					// Get production
					std::vector<std::size_t> production;
					for (size_t i = 0; i < action.pop_count * 2; ++i)
					{
						if (i % 2 == 1)
						{
							production.push_back(reduction_stack.back());
						}

						reduction_stack.pop_back();
					}

					production = std::vector(std::rbegin(production), std::rend(production));

					// Get arguments for the action
					std::vector<std::string> values;
					for(std::size_t i = 0; i < action.pop_count; ++i)
					{
						values.push_back(value_stack.back());
						value_stack.pop_back();
					}

					values = std::vector(std::rbegin(values), std::rend(values));

					std::size_t new_state = -1;
					auto n = action.push_state;

					state_id = reduction_stack.back();
					reduction_stack.push_back(n);
					bool is_done = (parser[state_id].next().contains(n) == false);

					if(is_done == false)
						new_state = parser[state_id].next().at(n);

					// Find the satisfied production
					auto& nt = std::get<
						parser_compiler::parser_compiler_result::non_terminal>(parser_.tokens[n]);

					value_stack.emplace_back();
					for (std::size_t j = 0; j < std::size(nt.productions); ++j)
					{
						if (nt.productions[j] == production)
						{
							const auto& action_name = nt.production_actions[j];
							if (std::empty(action_name))
								break;

							try
							{
								value_stack.back() =
									this->actions_.at(action_name)(values);
							}
							catch (const std::out_of_range&)
							{
								throw std::logic_error("Undefined action.");
							}

							break;
						}
					}

					if(is_done == false)
					{
						reduction_stack.push_back(new_state);
					}
					modified = ( is_done == false );	
					
				}
				else if(std::holds_alternative<parser_compiler::parser_compiler_result::state_data::action_accept>(r->second))
				{
					modified = false;
				}
				else
				{
					assert(false);
				}

				for (std::size_t i = 0; i < reduction_stack.size(); ++i)
				{
					if(i % 2 == 0)
						os << reduction_stack[i] << ' ';
					else
						os << "[" << reduction_stack[i] << "] ";
				}
				os << '\n';
			}

			return value_stack.back();
		}
	};
}
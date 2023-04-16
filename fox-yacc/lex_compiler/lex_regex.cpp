#include <variant>
#include <memory>
#include <list>
#include <cassert>
#include <functional>

#include <automata/NFA.hpp>

#include "lex_compiler.hpp"

using namespace fox_cc;

using charset = lex_compiler::charset;

enum class regex_parser_op
{
	closure_zero_more,
	closure_one_more,
	closure_zero_one,
	concatenation,
	alternative,
	left_parenthesis,
	right_parenthesis
};

[[nodiscard]] inline size_t regex_parser_op_priority(regex_parser_op op)
{
	using enum regex_parser_op;
	switch (op)
	{
	case closure_one_more:
	case closure_zero_more:
	case closure_zero_one:
		return 2;
	case concatenation:
		return 1;
	case alternative:
	case left_parenthesis:
	case right_parenthesis:
		return 0;
	default:
		assert(false);
		return 0;
	}
}

struct regex_end {};

using regex_parser_token = std::variant<std::monostate, charset, regex_parser_op, regex_end>;

// converts regex to reverse polish notation regular expression, resolves character classes
class regex_parser
{
	std::string_view str_;
	size_t lexer_current_char_pos_ = {};
	char lexer_current_char_ = std::empty(str_) ? '\0' : str_[lexer_current_char_pos_];

	regex_parser_token c0_ = {};
	regex_parser_token c1_ = {};

public:
	regex_parser() = delete;
	regex_parser(const regex_parser&) = delete;
	regex_parser(regex_parser&&) noexcept = delete;
	regex_parser& operator=(const regex_parser&) = delete;
	regex_parser& operator=(regex_parser&&) noexcept = delete;

public:
	regex_parser(std::string_view str) noexcept
		: str_(str)
	{}

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
	void error(const char* error_string);

private:
	// converts to reverse polish notation!
	std::vector<regex_parser_token> compile_rpn();

private:

	using nfa = fox_cc::automata::nfa<automata::empty_state, charset>;
	
	static nfa nfa_empty_expression();
	static nfa nfa_charset_expression(charset ch);
	static nfa nfa_concatenation_expression(const nfa& lhs, const nfa& rhs);
	static nfa nfa_union_expression(const nfa& lhs, const nfa& rhs);
	static nfa nfa_closure_one_more_expression(const nfa& expr);
	static nfa nfa_closure_zero_one_expression(const nfa& expr);
	static nfa nfa_closure_zero_more_expression(const nfa& expr);

	nfa compile_nfa(const std::vector<regex_parser_token>& rpn);

public:
	void compile();
};

regex_parser_token regex_parser::token()
{
	if(std::holds_alternative<std::monostate>(c0_))
	{
		c0_ = lexer_token();
		c1_ = lexer_token();
	}

	const auto ret = c0_;

	// Insert fake concatenation ops
	if(
		std::holds_alternative<charset>(c0_) && std::holds_alternative<charset>(c1_) ||
		std::holds_alternative<regex_parser_op>(c0_) && std::get<regex_parser_op>(c0_) == regex_parser_op::right_parenthesis && std::holds_alternative<charset>(c1_) ||
		std::holds_alternative<charset>(c0_) && std::holds_alternative<regex_parser_op>(c1_) && std::get<regex_parser_op>(c1_) == regex_parser_op::left_parenthesis
		)
	{
		c0_ = regex_parser_op::concatenation;
	}
	else
	{
		c0_ = c1_;
		c1_ = lexer_token();
	}

	return ret;
}

regex_parser_token regex_parser::lexer_token()
{
	regex_parser_token out;

	[[maybe_unused]] bool b = 
		lexer_escape_char(out) || 
		lexer_operator(out) || 
		lexer_char_group(out) || 
		lexer_end(out) ||
		lexer_char(out) ||
		lexer_error(out);

	return out;
}

bool regex_parser::lexer_escape_char(regex_parser_token& t)
{
	if (this->lexer_char() != '\\')
		return false;

	this->lexer_next_char();

	switch (this->lexer_char())
	{
	case '(':
	case ')':
	case '[':
	case ']':
	case '?':
	case '+':
	case '*':
	case '|':
	case '/':
		t = charset{}.set(this->lexer_char());
		break;
	case 'n':
		t = charset{}.set('\n');
		break;
	case 't':
		t = charset{}.set('\t');
		break;
	case 'd': // decimal characters
		t = charset{}.set('0').set('1').set('2').set('3').set('4').set('5').set('6').set('7').set('8').set('9');
		break;
	default:
		error("Unknown escape character."); // TODO: Which escape character!
		return false;
	}

	this->lexer_next_char();
	return false;
}

bool regex_parser::lexer_char_group(regex_parser_token& t)
{
	if (this->lexer_char() != '[')
		return false;

	this->lexer_next_char();

	charset out;
	bool in_escape = false;

	char range_start = '\0';
	bool in_range = false;

	while(this->lexer_char() != ']')
	{
		if (this->lexer_char() == '\0')
			error("Missing character group closing bracket.");

		if(in_escape == false && this->lexer_char() == '\\')
		{
			in_escape = true;
			this->lexer_next_char();
			if (this->lexer_char() != '-' && this->lexer_char() != ']')
				error("Invalid escape character inside a character group."); // Which character!

			continue;
		}

		if (!in_escape && this->lexer_char() == '-')
		{
			in_range = true;
		}
		else if(in_range == true)
		{
			if(range_start == '\0')
				error("Mismatched character group range '-'.");

			if(this->lexer_char() == '-' && !in_escape)
				error("Mismatched character group range '-'.");

			// TODO: Error bad range?
			const auto min = std::min(range_start, this->lexer_char());
			const auto max = std::max(range_start, this->lexer_char());

			for (size_t i = min; i <= max; ++i)
				out.set(i);

			range_start = '\0';
			in_range = false;
		}
		else
		{
			range_start = this->lexer_char();
			out.set(this->lexer_char());
		}

		this->lexer_next_char();
		in_escape = false;
	}

	if (in_escape == true)
		error("Mismatched escape \\ character.");

	if (in_range == true)
		error("Mismatched character group range '-'.");

	this->lexer_next_char();

	t = out;
	return true;
}

bool regex_parser::lexer_char(regex_parser_token& t)
{
	t = charset{}.set(this->lexer_char());
	this->lexer_next_char();
	return true;
}

bool regex_parser::lexer_operator(regex_parser_token& t)
{
	switch (this->lexer_char())
	{
	case '?':
		t = regex_parser_op::closure_zero_one;
		break;
	case '*':
		t = regex_parser_op::closure_zero_more;
		break;
	case '+':
		t = regex_parser_op::closure_one_more;
		break;
	case '(':
		t = regex_parser_op::left_parenthesis;
		break;
	case ')':
		t = regex_parser_op::right_parenthesis;
		break;
	case '|':
		t = regex_parser_op::alternative;
		break;
	default:
		return false;
	}

	this->lexer_next_char();
	return true;
}

bool regex_parser::lexer_end(regex_parser_token& t)
{
	if (this->lexer_char() == '\0')
	{
		t = regex_end{};
		this->lexer_next_char();
		return true;
	}

	return false;
}

bool regex_parser::lexer_error(regex_parser_token& t)
{
	this->error("Unknown lexing symbol.");
	return false;
}

char regex_parser::lexer_char() const noexcept
{
	return lexer_current_char_;
}

void regex_parser::lexer_next_char() noexcept
{
	lexer_current_char_pos_ += 1;
	if (lexer_current_char_pos_ >= std::size(str_))
		lexer_current_char_ = '\0';
	else
		lexer_current_char_ = str_[lexer_current_char_pos_];
}

void regex_parser::error(const char* error_string)
{
	assert(false);
}

#include <iostream>

std::vector<regex_parser_token> regex_parser::compile_rpn()
{
	// Shunting yard algorithm to convert infix to postfix (RPN) notation REGEX
	std::vector<regex_parser_token> output;
	std::vector<regex_parser_token> operator_stack;

	for(regex_parser_token t = token(); !std::holds_alternative<regex_end>(t); t = token())
	{
		std::visit([](auto v)
			{
				if constexpr (std::is_same_v<decltype(v), regex_parser_op>)
				{
					using enum regex_parser_op;
					switch (v)
					{
					case closure_one_more:
					case closure_zero_more:
					case closure_zero_one:
						std::cout << "*\n";
						break;
					case concatenation:
						std::cout << "?\n";
						break;
					case alternative:
						std::cout << "|\n";
						break;
					case left_parenthesis:
						std::cout << "(\n";
						break;
					case right_parenthesis:
						std::cout << ")\n";
						break;
					}
				}
				else
				{
					std::cout << typeid(v).name() << '\n';
				}
			}, t);

		if(std::holds_alternative<charset>(t))
		{
			output.push_back(t);
		}
		else if(std::holds_alternative<regex_parser_op>(t) && std::get<regex_parser_op>(t) == regex_parser_op::left_parenthesis)
		{
			operator_stack.push_back(t);
		}
		else if(std::holds_alternative<regex_parser_op>(t) && std::get<regex_parser_op>(t) == regex_parser_op::right_parenthesis)
		{
			do
			{
				if(std::empty(operator_stack))
				{
					error("Mismatched parenthesis.");
				}

				auto op = operator_stack.back();
				operator_stack.pop_back();

				if(std::get<regex_parser_op>(op) == regex_parser_op::left_parenthesis)
				{
					break;
				}

				output.push_back(op);

			} while (true);
		}
		else if(std::holds_alternative<regex_parser_op>(t))
		{
			while(!std::empty(operator_stack) && // stack is not empty
				std::get<regex_parser_op>(operator_stack.back()) != regex_parser_op::left_parenthesis && // operator on the top of the stack is not left-parenthesis
				regex_parser_op_priority(std::get<regex_parser_op>(operator_stack.back())) >= regex_parser_op_priority(std::get<regex_parser_op>(t)) // op has greater or equal precedence
				)
			{
				// pop operator from the queue and push into output queue
				output.push_back(operator_stack.back());
				operator_stack.pop_back();
			}

			// push t to operator stack
			operator_stack.push_back(t);
		}
	}

	while (!std::empty(operator_stack))
	{
		auto op = operator_stack.back();
		operator_stack.pop_back();

		if (std::get<regex_parser_op>(op) == regex_parser_op::left_parenthesis)
			error("Mismatched parenthesis.");

		output.push_back(op);
	}

	return output;
}

regex_parser::nfa regex_parser::nfa_empty_expression()
{
	nfa out;
	const auto start = out.insert();
	const auto end = out.insert();
	out.start() = start;
	out.accept().insert(end);
	out.connect(start, end);
	return out;
}

regex_parser::nfa regex_parser::nfa_charset_expression(charset ch)
{
	nfa out;
	const auto start = out.insert();
	const auto end = out.insert();
	out.start() = start;
	out.accept().insert(end);
	out.connect(start, end, ch);
	return out;
}

regex_parser::nfa regex_parser::nfa_concatenation_expression(const nfa& lhs, const nfa& rhs)
{
	nfa out = lhs;
	const auto map = out.insert(rhs);

	// Update accepts
	out.accept().clear();
	for(auto e : rhs.accept())
		out.accept().insert(map[e]);

	// Stitch rhs start with lhs ends
	const auto old_start = map[rhs.start()];
	for(auto e : lhs.accept())
		out.connect(e, old_start);

	return out;
}

regex_parser::nfa regex_parser::nfa_union_expression(const nfa& lhs, const nfa& rhs)
{
	nfa out;
	const auto start = out.insert();
	const auto end = out.insert();
	out.start() = start;
	out.accept().insert(end);

	// connect lhs and rhs
	for(const auto r = { std::addressof(lhs), std::addressof(rhs) }; const auto ptr : r)
	{
		const auto& expr = *ptr;
		const auto map = out.insert(expr);

		out.connect(start, map[expr.start()]);
		for (const auto e : expr.accept())
			out.connect(map[e], end);
	}

	return out;
}

regex_parser::nfa regex_parser::nfa_closure_one_more_expression(const nfa& expr)
{
	nfa out = expr;

	for(const auto e : out.accept())
		out.connect(e, out.start());

	return out;
}

regex_parser::nfa regex_parser::nfa_closure_zero_one_expression(const nfa& expr)
{
	nfa out = expr;

	for (const auto e : out.accept())
		out.connect(out.start(), e);
	
	return out;
}

regex_parser::nfa regex_parser::nfa_closure_zero_more_expression(const nfa& expr)
{
	nfa out = expr;

	for (const auto e : out.accept())
	{
		out.connect(out.start(), e);
		out.connect(e, out.start());
	}

	return out;
}

regex_parser::nfa regex_parser::compile_nfa(const std::vector<regex_parser_token>& rpn)
{
	std::vector<nfa> nfa_stack;
	nfa_stack.reserve(2);

	for(const auto& token : rpn)
	{
		if(std::holds_alternative<charset>(token))
		{
			nfa_stack.push_back(nfa_charset_expression(std::get<charset>(token)));
		}
		else if(std::holds_alternative<regex_parser_op>(token))
		{
			auto op = std::get<regex_parser_op>(token);

			using enum regex_parser_op;
			switch (op)
			{
			case alternative:
			{
				assert(std::size(nfa_stack) >= 2);
				auto rhs = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				auto lhs = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_union_expression(lhs, rhs));
				break;
			}
			case concatenation:
			{
				assert(std::size(nfa_stack) >= 2);
				auto rhs = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				auto lhs = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_concatenation_expression(lhs, rhs));
				break;
			}
			case closure_one_more:
			{
				assert(std::size(nfa_stack) >= 1);
				auto expr = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_closure_one_more_expression(expr));
				break;
			}
			case closure_zero_more:
			{
				assert(std::size(nfa_stack) >= 1);
				auto expr = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_closure_zero_more_expression(expr));
				break;
			}
			case closure_zero_one:
			{
				assert(std::size(nfa_stack) >= 1);
				auto expr = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_closure_zero_one_expression(expr));
				break;
			}
			default:
				assert(false);
				break;
			}
		}
		else
		{
			assert(false);
		}
	}

	assert(std::size(nfa_stack) == 1);

	return nfa_stack[0];
}

void regex_parser::compile()
{
	const auto rpn_regex = this->compile_rpn();
	auto nfa = this->compile_nfa(rpn_regex);
	bool f = true;
}


fox_cc::lex_compiler::regex_productions fox_cc::lex_compiler::generate_production(std::string_view regex)
{
	regex_parser rp(regex);
	
	rp.compile();

	return {};
}

void fox_cc::lex_compiler::populate_first_sets(regex_productions& prods)
{

}
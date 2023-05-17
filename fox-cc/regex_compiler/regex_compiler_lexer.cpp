#include <regex_compiler/regex_compiler.hpp>

fox_cc::regex_compiler::regex_parser::regex_parser_token fox_cc::regex_compiler::regex_parser::token()
{
	if (std::holds_alternative<std::monostate>(c0_))
	{
		c0_ = lexer_token();
		c1_ = lexer_token();
	}

	const auto ret = c0_;

	// Insert fake concatenation ops
	if (
		std::holds_alternative<charset>(c0_) && std::holds_alternative<charset>(c1_) ||
		std::holds_alternative<regex_parser_token_right_parenthesis>(c0_) && std::holds_alternative<charset>(c1_) ||
		std::holds_alternative<charset>(c0_) && std::holds_alternative<regex_parser_token_left_parenthesis>(c1_)
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

fox_cc::regex_compiler::regex_parser::regex_parser_token fox_cc::regex_compiler::regex_parser::lexer_token()
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

bool fox_cc::regex_compiler::regex_parser::lexer_escape_char(regex_parser_token& t)
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
		throw_error("Unknown escape character."); // TODO: Which escape character!
		return false;
	}

	this->lexer_next_char();
	return false;
}

bool fox_cc::regex_compiler::regex_parser::lexer_char_group(regex_parser_token& t)
{
	if (this->lexer_char() != '[')
		return false;

	this->lexer_next_char();

	charset out;
	bool in_escape = false;

	char range_start = '\0';
	bool in_range = false;

	while (this->lexer_char() != ']')
	{
		if (this->lexer_char() == '\0')
			throw_error("Missing character group closing bracket.");

		if (in_escape == false && this->lexer_char() == '\\')
		{
			in_escape = true;
			this->lexer_next_char();
			if (this->lexer_char() != '-' && this->lexer_char() != ']')
				throw_error("Invalid escape character inside a character group."); // Which character!

			continue;
		}

		if (!in_escape && this->lexer_char() == '-')
		{
			in_range = true;
		}
		else if (in_range == true)
		{
			if (range_start == '\0')
				throw_error("Mismatched character group range '-'.");

			if (this->lexer_char() == '-' && !in_escape)
				throw_error("Mismatched character group range '-'.");

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
		throw_error("Mismatched escape \\ character.");

	if (in_range == true)
		throw_error("Mismatched character group range '-'.");

	this->lexer_next_char();

	t = out;
	return true;
}

bool fox_cc::regex_compiler::regex_parser::lexer_char(regex_parser_token& t)
{
	t = charset{}.set(this->lexer_char());
	this->lexer_next_char();
	return true;
}

bool fox_cc::regex_compiler::regex_parser::lexer_operator(regex_parser_token& t)
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
		t = regex_parser_token_left_parenthesis{};
		break;
	case ')':
		t = regex_parser_token_right_parenthesis{};
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

bool fox_cc::regex_compiler::regex_parser::lexer_end(regex_parser_token& t)
{
	if (this->lexer_char() == '\0')
	{
		t = regex_parser_token_end{};
		this->lexer_next_char();
		return true;
	}

	return false;
}

bool fox_cc::regex_compiler::regex_parser::lexer_error(regex_parser_token& t)
{
	this->throw_error("Unknown lexing symbol.");
	return false;
}

char fox_cc::regex_compiler::regex_parser::lexer_char() const noexcept
{
	return lexer_current_char_;
}

void fox_cc::regex_compiler::regex_parser::lexer_next_char() noexcept
{
	lexer_current_char_pos_ += 1;
	if (lexer_current_char_pos_ >= std::size(str_))
		lexer_current_char_ = '\0';
	else
		lexer_current_char_ = str_[lexer_current_char_pos_];
}
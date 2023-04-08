#include <cassert>
#include <set>

#include "lexer.hpp"

prs::token_entry prs::lexer::next_token()
{
	token_entry out;

	while (in_range() && (consume_space() || consume_comment()));

	if (in_range())
	{
		[[maybe_unused]] bool v =
			interpret_identifier(out) ||
			interpret_command(out) ||
			interpret_literal(out) ||
			interpret_number(out) ||
			interpret_tag(out) ||
			interpret_c_action(out) ||
			interpret_colon(out) ||
			interpret_semicolon(out) ||
			interpret_comma(out) ||
			interpret_or(out) ||
			generate_invalid_token(out);

		return out;
	}

	this->push_state();
	build_token_entry(token::END_OF_FILE, out);
	return out;
}

prs::token_entry prs::lexer::next_regex_token()
{
	token_entry out;

	while (in_range() && (consume_space() || consume_comment()));

	if(in_range())
	{
		interpret_regex(out);
		return out;
	}

	this->push_state();
	build_token_entry(token::END_OF_FILE, out);
	return out;
}

prs::token_entry prs::lexer::c_definition_token()
{
	prs::token_entry out;
	current_ = std::size(input_);
	build_token_entry(token::C_DEFINITION, out);
	return out;
}

void prs::lexer::marker_forward()
{
	assert_marker_in_range();

	if(input_[current_] == '\n')
	{
		debug_line_ += 1;
		debug_col_ = 0;
	}
	else
	{
		debug_col_ += 1;
	}

	current_ += 1;
	if (in_range())
		c_ = input_[current_];
	else
		c_ = '\0';
}

void prs::lexer::assert_marker_in_range()
{
	assert(in_range());
}

char prs::lexer::c() const
{
	return c_;
}

bool prs::lexer::in_range() const
{
	return current_ < std::size(input_);
}

void prs::lexer::push_state()
{
	pushed_current_ = current_;
	pushed_debug_line_ = debug_line_;
	pushed_debug_col_ = debug_col_;
}

bool prs::lexer::pop_state()
{
	current_ = pushed_current_;
	debug_line_ = pushed_debug_line_;
	debug_col_ = pushed_debug_col_;
	c_ = input_[current_];
	return false;
}

bool prs::lexer::build_token_entry(token type, token_entry& entry, std::string_view sv, int v)
{
	std::string_view tkn(
		std::data(input_) + pushed_current_,
		current_ - pushed_current_
	);

	auto& info = tokens_.emplace_back();
	info.token = tkn;
	info.line = static_cast<uint32_t>(debug_line_);
	info.column = static_cast<uint32_t>(debug_col_);
	info.string_value = std::empty(sv) ? tkn : sv;
	info.int_value = v;

	entry.value = type;
	entry.info = std::addressof(info);

	return true;
}

bool prs::lexer::consume_space()
{
	assert_marker_in_range();

	while(in_range())
	{
		if (!std::isspace(c()))
			return false;

		this->marker_forward();
	}

	return true;
}

bool prs::lexer::consume_comment()
{
	assert_marker_in_range();
	push_state();

	if (c() != '/') // both comments start with /
		return pop_state();

	this->marker_forward();

	if(c()  == '/') // first type goes till new-line
	{
		this->marker_forward();

		while (in_range())
		{
			if(c() == '\n') // consume it and break
			{
				this->marker_forward();
				return true;
			}
		}
	}
	else if(c()  == '*')
	{
		char prev = c();
		this->marker_forward();

		while(in_range())
		{
			if(prev == '*' && c() == '/') // 2nd one goes till */ closing comment
			{
				this->marker_forward();
				return true;
			}

			prev = c();
			this->marker_forward();
		}
	}
	else
	{
		return pop_state();
	}

	return true; // EOF is also fine
}

bool prs::lexer::interpret_number(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	if (!std::isdigit(c()))
		return pop_state();

	this->marker_forward();

	int v = 0;

	while (in_range())
	{
		if (std::isdigit(c()))
		{
			v *= 10;
			v += (c() - '0');
		}
		else if(std::isspace(c()) || c() == '<' || c() == '%' || c() == ',' || c() == ';' || c() == ':')
		{
			break;
		}
		else
		{
			return pop_state();
		}
	}

	return build_token_entry(token::NUMBER, entry, {}, v);
}

bool prs::lexer::interpret_identifier(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	if (!std::isalpha(c()))
		return pop_state();

	this->marker_forward();

	while(in_range())
	{
		if(std::isalnum(c()) || c() == '_')
		{
			this->marker_forward();
		}
		else
		{
			break;
		}
	}

	return build_token_entry(token::IDENTIFIER, entry);
}

bool prs::lexer::interpret_regex(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	if (std::isspace(c()))
		return pop_state();

	while (!std::isspace(c()))
		this->marker_forward();

	return build_token_entry(token::REGEX, entry);
}

bool prs::lexer::interpret_literal(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	if (c() != '\'')
		return pop_state();

	this->marker_forward();
	char marker = c();

	// TODO: Implement escape characters
	this->marker_forward();

	if (c() != '\'')
		return pop_state();

	this->marker_forward();
	return build_token_entry(token::LITERAL, entry, {}, marker);
}

bool prs::lexer::interpret_tag(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	if (c() != '<')
		return pop_state();

	consume_space();

	if (!std::isalpha(c()))
		return pop_state();

	const char* tag_start = std::data(input_) + current_;
	size_t tag_size = 1;
	this->marker_forward();

	while (in_range())
	{
		if (std::isalnum(c()) || c() == '_')
		{
			this->marker_forward();
			tag_size += 1;
		}
		else
		{
			break;
		}
	}

	consume_space();
	if (c() != '>')
		return pop_state();

	this->marker_forward();
	return build_token_entry(token::TAG, entry, std::string_view(tag_start, tag_size));
}

bool prs::lexer::interpret_c_action(token_entry& entry)
{
	// TODO: Account for braces inside comments

	assert_marker_in_range();
	push_state();

	if (c() != '{')
		return pop_state();

	this->marker_forward();
	size_t depth = 0;

	while (in_range())
	{
		if (c() == '{')
			depth += 1;

		if(c() == '}')
		{
			if (depth)
				depth -= 1;
			else
				break;
		}

		this->marker_forward();
	}

	this->marker_forward();
	return build_token_entry(token::C_ACTION, entry);
}

bool prs::lexer::interpret_colon(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	if (c() != ':')
		return pop_state();

	this->marker_forward();
	return build_token_entry(token::COLON, entry);
}

bool prs::lexer::interpret_semicolon(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	if (c() != ';')
		return pop_state();

	this->marker_forward();
	return build_token_entry(token::SEMICOLON, entry);
}

bool prs::lexer::interpret_comma(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	if (c() != ',')
		return pop_state();

	this->marker_forward();
	return build_token_entry(token::COMMA, entry);
}

bool prs::lexer::interpret_or(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	if (c() != '|')
		return pop_state();

	this->marker_forward();
	return build_token_entry(token::OR, entry);
}

bool prs::lexer::interpret_command(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	constexpr size_t hash_type = 't' + 'y' + 'p' + 'e';
	constexpr size_t hash_left = 'l' + 'e' + 'f' + 't';
	constexpr size_t hash_right = 'r' + 'i' + 'g' + 'h' + 't';
	constexpr size_t hash_nonassoc = 'n' + 'o' + 'n' + 'a' + 's' + 's' + 'o' + 'c' + 'k';
	constexpr size_t hash_token = 't' + 'o' + 'k' + 'e' + 'n';
	constexpr size_t hash_prec = 'p' + 'r' + 'e' + 'c';
	constexpr size_t hash_start = 's' + 't' + 'a' + 'r' + 't';
	constexpr size_t hash_variant = 'v' + 'a' + 'r' + 'i' + 'a' + 'n' + 't';

	assert((std::set<size_t>
		{
			hash_type, hash_left, hash_right, hash_nonassoc,
			hash_token, hash_prec, hash_start, hash_variant
		}.size() == 8));

 	if (c() != '%')
		return pop_state();

	this->marker_forward();

	if(c() == '%')
	{
		this->marker_forward();
		return build_token_entry(token::MARK, entry);
	}
	else if (c() == '{')
	{
		this->marker_forward();

		const char* body_start = std::data(input_) + current_;
		size_t body_size = 0;
		char marker = c();
		this->marker_forward();

		while(marker != '%' && c() != '}')
		{
			if (!in_range())
				return pop_state();

			body_size += 1;
			marker = c();
			this->marker_forward();
		}

		return build_token_entry(token::C_DECLARATION, entry, std::string_view{body_start, body_size});
	}

	size_t hash = 0;
	for(size_t i = 0; i < 10 && in_range(); ++i) // We want to quit on white space, 
		// that's why we pick 10 instead of 9 - longest string
	{
		if (std::isspace(c()) || c() == '<' || c() == ',' || c() == ';')
			break;

		hash += c();
		this->marker_forward();
	}

	switch (hash)
	{
	case hash_type:
		return build_token_entry(token::TYPE, entry);
	case hash_left:
		return build_token_entry(token::LEFT, entry);
	case hash_right:
		return build_token_entry(token::RIGHT, entry);
	case hash_nonassoc:
		return build_token_entry(token::NONASSOC, entry);
	case hash_token:
		return build_token_entry(token::TOKEN, entry);
	case hash_prec:
		return build_token_entry(token::PREC, entry);
	case hash_start:
		return build_token_entry(token::START, entry);
	case hash_variant:
		return build_token_entry(token::VARIANT, entry);
	default:;
	}

	return pop_state();
}

bool prs::lexer::generate_invalid_token(token_entry& entry)
{
	assert_marker_in_range();
	push_state();

	while (in_range())
	{
		if (std::isspace(c()))
			break;
		
		this->marker_forward();
	}

	return build_token_entry(token::INVALID_TOKEN, entry);
}
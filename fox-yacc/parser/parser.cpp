#include "parser.hpp"

void prs::parser::next_token()
{
	e0_ = e1_;
	e1_ = lexer_.next_token();
}

prs::token_entry prs::parser::e0() const noexcept
{
	return e0_;
}

prs::token_entry prs::parser::e1() const noexcept
{
	return e1_;
}

prs::yacc_ast prs::parser::ast() const noexcept
{
	return ast_;
}

void prs::parser::parse()
{
	parse_spec();
}

void prs::parser::parse_spec()
{
	parse_defs();
	parse_prods();
	parse_tail();
	expect(token::END_OF_FILE);
}

void prs::parser::parse_defs()
{
	while(e0() != token::MARK)
		parse_def();
}

void prs::parser::parse_prods()
{
	expect(token::MARK);
	next_token();

	while (e0() != token::MARK && e0() != token::END_OF_FILE)
		parse_prod();
}

void prs::parser::parse_tail()
{
	expect(token::END_OF_FILE, token::MARK);

	if (e0()  == token::END_OF_FILE)
		return;

	lexer_.c_definition_token();
	next_token();
}

void prs::parser::parse_def()
{
	using enum token;

	switch (static_cast<token>(e0()))
	{
	case START:
		parse_def_start();
		break;
	case UNION:
		parse_def_union();
		break;
	case LEFT:
	case RIGHT:
	case NONASSOC:
	case TOKEN:
		parse_def_token();
		break;
	case TYPE:
		parse_def_type();
		break;
	case C_DECLARATION:
		parse_def_c_decl();
		break;
	case MARK:
		break;
	default:
		error();
		break;
	}
}

void prs::parser::parse_def_start()
{
	expect(token::START);
	next_token();

	expect(token::IDENTIFIER);
	if (static_cast<bool>(ast_.start_identifier) == true)
		error();
	ast_.start_identifier = e0();

	next_token();
}

void prs::parser::parse_def_union()
{
	expect(token::UNION);
	next_token();

	expect(token::C_ACTION);

	if (static_cast<bool>(ast_.union_identifier) == true)
		error();
	ast_.union_identifier = e0();

	next_token();
}

void prs::parser::parse_def_token()
{
	using enum token;

	expect(LEFT, RIGHT, NONASSOC, TOKEN);
	yacc_ast::definition def
	{
		.rword = e0()
	};
	next_token();

	if(e0() == token::TAG)
	{
		def.tag = e0();
		next_token();
	}

	expect(token::IDENTIFIER);
	do
	{
		auto& token = def.tokens.emplace_back();
		token.name = e0();

		next_token();

		if (e0() == token::NUMBER)
		{
			token.number = e0();
			next_token();
		}

	} while (e0() == token::IDENTIFIER);

	ast_.token_definitions.push_back(std::move(def));
}

void prs::parser::parse_def_type()
{
	expect(token::TYPE);
	next_token();

	expect(token::TAG);
	auto tag = e0();
	next_token();

	expect(token::IDENTIFIER);
	do
	{
		auto r = ast_.name_type.insert(
			std::make_pair(e0(), tag)
		);
		next_token();

	} while (e0() != token::IDENTIFIER);
}

void prs::parser::parse_def_c_decl()
{
	expect(token::C_DECLARATION);
	ast_.c_declarations.push_back(e0());
	next_token();
}

void prs::parser::parse_prod()
{
	using enum token;

	expect(IDENTIFIER);
	yacc_ast::production prod
	{
		.name = e0()
	};
	next_token();

	expect(COLON);
	next_token();

	while (true)
	{
		expect(OR, SEMICOLON, IDENTIFIER, LITERAL);

		yacc_ast::production::rule rule;

		if (e0() == IDENTIFIER)
		{
			rule.tokens.push_back(e0());
			next_token();
		}

		while (e0() == IDENTIFIER || e0() == LITERAL || e0() == C_ACTION)
		{
			rule.tokens.push_back(e0());
			next_token();
		}

		prod.rules.push_back(std::move(rule));

		expect(SEMICOLON, OR);

		if (e0() == OR)
		{
			next_token();
			continue;
		}

		break;
	}
	
	expect(SEMICOLON);
	next_token();

	ast_.productions.push_back(std::move(prod));
}

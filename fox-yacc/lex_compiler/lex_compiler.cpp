#include "lex_compiler.hpp"

fox_cc::lexer fox_cc::lex_compiler::compile(prs::yacc_ast& ast)
{
	for(const auto& def : ast.token_definitions)
	{
		(void)this->generate_production(def.regex.info->string_value);
	}

	return {};
}
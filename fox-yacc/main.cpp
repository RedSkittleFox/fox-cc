#include <iostream>
#include <fstream>
#include <string>

#include <internal_parser/lexer.hpp>
#include <internal_parser/parser.hpp>
#include <parser_compiler/compiler.hpp>
#include <lex_compiler/lex_compiler.hpp>

int main()
{
	std::fstream file("test.yacc");
	std::string input(
		std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{}
	);

	prs::lexer lx(input);

	prs::parser ps(lx);

	ps.parse();
	auto ast = ps.ast();

	fox_cc::lex_compiler lex_cmp(ast);
	auto& lex_compiler_result = lex_cmp.result();
	// cmp::compiler cp(ast);
	// cp.compile();

	// cp.debug_print();

	return 0;
}
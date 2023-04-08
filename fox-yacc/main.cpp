#include <iostream>
#include <fstream>
#include <string>

#include "parser/lexer.hpp"
#include "parser/parser.hpp"
#include "compiler/compiler.hpp"
#include "lex_compiler/lex_compiler.hpp"

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

	fox_cc::lex_compiler lex_cmp;
	lex_cmp.compile(ast);

	// cmp::compiler cp(ast);
	// cp.compile();

	// cp.debug_print();

	return 0;
}
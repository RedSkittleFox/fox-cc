#include <iostream>
#include <fstream>
#include <string>

#include "parser/lexer.hpp"
#include "parser/parser.hpp"

int main()
{
	std::fstream file("test.yacc");
	std::string input(
		std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}
	);

	prs::lexer lx(input);

	prs::parser ps(lx);

	ps.parse();
	auto ast = ps.ast();

	return 0;
}
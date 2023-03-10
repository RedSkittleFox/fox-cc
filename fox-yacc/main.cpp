#include <iostream>
#include <fstream>
#include <string>

#include "parser/lexer.hpp"

int main()
{
	std::fstream file("test.yacc");
	std::string input(
		std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}
	);

	prs::lexer lx(input);

	while(true)
	{
		auto token = lx.next_token();

		std::cout << token.info->token << '\t' << token.info->string_value << '\n';

		if (token.value == prs::token::END_OF_FILE || token.value == prs::token::INVALID_TOKEN)
			break;
	}

	return 0;
}
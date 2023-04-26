#include <iostream>
#include <fstream>
#include <string>

#include <internal_parser/lexer.hpp>
#include <internal_parser/parser.hpp>
#include <lex_compiler/lex_compiler.hpp>
#include <parser_compiler/parser_compiler.hpp>

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
	fox_cc::parser_compiler prs_cmp(lex_compiler_result, ast);
	auto& result = prs_cmp.result();

	for(size_t i = 0; i < result.dfa.size(); ++i)
	{
		auto& state = result.dfa[i];

		std::cout << "===== " << i << "=====\n";

		for(auto prod : state.value().productions)
		{
			auto& nt = result.tokens[prod.non_terminal].non_terminal();

			std::cout << nt.name << " -> ";

			auto& sub_prod = nt.productions[prod.non_terminal_production];

			for(size_t i = 0; i <= sub_prod.size(); ++i)
			{
				if (i == prod.current)
					std::cout << ". ";

				if(i < sub_prod.size())
					std::cout << result.tokens[sub_prod[i]].name() << " ";
			}

			std::cout << " [ ";
			for (auto follow : prod.follow_set)
				std::cout << result.tokens[follow].name() << ' ';
			std::cout << "]\n";

		}

		std::cout << "\n";

		for(auto go_to : state.next())
		{
			std::cout << result.tokens[go_to.first].name() << " : " << go_to.second << '\n';
		}

		std::cout << "\n";
	}

	// cmp::compiler cp(ast);
	// cp.compile();

	// cp.debug_print();

	return 0;
}
#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#include <fox_cc.hpp>

int main()
{
	std::fstream file("test.yacc");
	std::string input(
		std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{}
	);

	fox_cc::compiler cmp(input);

	auto& result = cmp.parser();
	std::cout << "digraph G {\n";
	for(size_t i = 0; i < result.dfa.size(); ++i)
	{
		auto& state = result.dfa[i];
		std::cout << i << " [ label = \"";
		std::cout << i << "\n";
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

			std::cout << " ( ";
			for (auto follow : prod.follow_set)
				std::cout << result.tokens[follow].name() << ' ';
			std::cout << ")\n";

		}
		std::cout << "\"];\n";

		for(auto go_to : state.next())
		{
			std::cout << i << " -> " << go_to.second << "[label=\"" << result.tokens[go_to.first].name() << " : " << go_to.first << "\"];\n";
		}

		std::cout << "\n";
	}

	std::cout << "}";

	auto ast = cmp.compile("1+2+3");
	

	// cmp::compiler cp(ast);
	// cp.compile();

	// cp.debug_print();

	return 0;
}
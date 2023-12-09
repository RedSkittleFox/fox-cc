#include <iostream>
#include <fstream>
#include <string>

#include <fox_cc.hpp>

int main()
{
	std::string grammar = R"(
%token	NUMBER		[0-9]+
%token	OP_ADD		[+]
%token	OP_SUB		[\-]
%token	OP_MUL		[*]
%token	OP_DIV		[/]
%token	OP_MOD		[%]
%token	L_PARENT	[(]
%token	R_PARENT	[)]

%start start

%%
start
	:	expression	{ forward }
	;

expression
	:	precedence_2 { forward }
	;

precedence_2
	:	precedence_2 OP_ADD precedence_1 { bin_op }
	|	precedence_2 OP_SUB precedence_1 { bin_op }
	|	precedence_1					 { forward }
	;

precedence_1
	:	precedence_1 OP_MUL precedence_0 { bin_op }
	|	precedence_1 OP_DIV precedence_0 { bin_op }
	|	precedence_1 OP_MOD precedence_0 { bin_op }
	|	precedence_0					 { forward }
	;

precedence_0
	:	NUMBER							{ forward }
	|	L_PARENT expression R_PARENT	{ parent_exp }
	;
%%
)";

	fox_cc::compiler cmp(grammar);

	cmp.register_action("forward", [](std::span<std::string> v)
	{
		return v[0];
	});
	cmp.register_action("bin_op", [](std::span<std::string> v)
	{
		auto lhs = std::stoi(v[0]);
		auto rhs = std::stoi(v[2]);
		int out;
		switch (v[1][0])
		{
		case '+':
			out = lhs + rhs; break;
		case '-':
			out = lhs - rhs; break;
		case '*':
			out = lhs * rhs; break;
		case '/':
			out = lhs / rhs; break;
		case '%':
			out = lhs % rhs; break;
		default:
			throw std::logic_error("Unknown operator.");
		}

		return std::to_string(out);
	});

	cmp.register_action("parent_exp", [](std::span<std::string> v) { return v[1]; });

	{
		std::fstream f("lexer_debug.txt", std::ios::trunc | std::ios::out );
		f << cmp.lexer_to_string();

		f = std::fstream("parser_debug.txt", std::ios::trunc | std::ios::out);
		f << cmp.parser_to_string();

		f = std::fstream("dot_parser.txt", std::ios::trunc | std::ios::out);
		f << cmp.dot_to_string();
	}

	{
		auto f = std::fstream("trace.txt", std::ios::trunc | std::ios::out);
		std::string op = "1+2*(2+2)";
		const auto computation_result = cmp.compile(op, f);
		std::cout << op << "=" << computation_result << '\n';

	}
	
	return 0;
}
#include <regex_compiler/regex_compiler.hpp>

#include <iostream>

size_t fox_cc::regex_compiler::regex_parser::regex_parser_op_priority(regex_parser_op op)
{
	using enum regex_parser_op;
	switch (op)
	{
	case closure_one_more:
	case closure_zero_more:
	case closure_zero_one:
		return 2;
	case concatenation:
		return 1;
	case alternative:
		return 0;
	}

	return 0;
}

std::vector<fox_cc::regex_compiler::regex_parser::regex_parser_token> fox_cc::regex_compiler::regex_parser::compile_rpn()
{
	// Shunting yard algorithm to convert infix to postfix (RPN) notation REGEX
	std::vector<regex_parser_token> output;
	std::vector<regex_parser_token> operator_stack;

	for (regex_parser_token t = token(); !std::holds_alternative<regex_parser_token_end>(t); t = token())
	{
		/*
		std::visit([](auto v)
			{
				if constexpr (std::is_same_v<decltype(v), regex_parser_op>)
				{
					using enum regex_parser_op;
					switch (v)
					{
					case closure_one_more:
					case closure_zero_more:
					case closure_zero_one:
						std::cout << "*\n";
						break;
					case concatenation:
						std::cout << "?\n";
						break;
					case alternative:
						std::cout << "|\n";
						break;
					}
				}
				else
				{
					std::cout << typeid(v).name() << '\n';
				}
			}, t);
		*/

		if (std::holds_alternative<charset>(t))
		{
			output.push_back(t);
		}
		else if (std::holds_alternative<regex_parser_token_left_parenthesis>(t))
		{
			operator_stack.push_back(t);
		}
		else if (std::holds_alternative<regex_parser_token_right_parenthesis>(t))
		{
			do
			{
				if (std::empty(operator_stack))
				{
					throw_error("Mismatched parenthesis.");
				}

				auto op = operator_stack.back();
				operator_stack.pop_back();

				if (std::holds_alternative<regex_parser_token_left_parenthesis>(op))
				{
					break;
				}

				output.push_back(op);

			} while (true);
		}
		else if (std::holds_alternative<regex_parser_op>(t))
		{
			while (!std::empty(operator_stack) && // stack is not empty
				!std::holds_alternative<regex_parser_token_left_parenthesis>(operator_stack.back()) && // operator on the top of the stack is not left-parenthesis
				regex_parser_op_priority(std::get<regex_parser_op>(operator_stack.back())) >= regex_parser_op_priority(std::get<regex_parser_op>(t)) // op has greater or equal precedence
				)
			{
				// pop operator from the queue and push into output queue
				output.push_back(operator_stack.back());
				operator_stack.pop_back();
			}

			// push t to operator stack
			operator_stack.push_back(t);
		}
	}

	while (!std::empty(operator_stack))
	{
		auto op = operator_stack.back();
		operator_stack.pop_back();

		if (std::holds_alternative<regex_parser_token_left_parenthesis>(op))
			throw_error("Mismatched parenthesis.");

		output.push_back(op);
	}

	return output;
}
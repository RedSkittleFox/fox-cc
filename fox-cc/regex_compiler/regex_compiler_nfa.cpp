#include <regex_compiler/regex_compiler.hpp>

fox_cc::regex_compiler::regex_parser::nfa fox_cc::regex_compiler::regex_parser::nfa_empty_expression()
{
	nfa out;
	const auto start = out.insert();
	const auto end = out.insert();
	out.start() = start;
	out.accept().insert(end);
	out.connect(start, end);
	return out;
}

fox_cc::regex_compiler::regex_parser::nfa fox_cc::regex_compiler::regex_parser::nfa_charset_expression(charset ch)
{
	nfa out;
	const auto start = out.insert();
	const auto end = out.insert();
	out.start() = start;
	out.accept().insert(end);
	out.connect(start, end, ch);
	return out;
}

fox_cc::regex_compiler::regex_parser::nfa fox_cc::regex_compiler::regex_parser::nfa_concatenation_expression(const nfa& lhs, const nfa& rhs)
{
	nfa out = lhs;
	const auto map = out.insert(rhs);

	// Update accepts
	out.accept().clear();
	for (auto e : rhs.accept())
		out.accept().insert(map[e]);

	// Stitch rhs start with lhs ends
	const auto old_start = map[rhs.start()];
	for (auto e : lhs.accept())
		out.connect(e, old_start);

	return out;
}

fox_cc::regex_compiler::regex_parser::nfa fox_cc::regex_compiler::regex_parser::nfa_union_expression(const nfa& lhs, const nfa& rhs)
{
	nfa out;
	const auto start = out.insert();
	const auto end = out.insert();
	out.start() = start;
	out.accept().insert(end);

	// connect lhs and rhs
	for (const auto r = { std::addressof(lhs), std::addressof(rhs) }; const auto ptr : r)
	{
		const auto& expr = *ptr;
		const auto map = out.insert(expr);

		out.connect(start, map[expr.start()]);
		for (const auto e : expr.accept())
			out.connect(map[e], end);
	}

	return out;
}

fox_cc::regex_compiler::regex_parser::nfa fox_cc::regex_compiler::regex_parser::nfa_closure_one_more_expression(const nfa& expr)
{
	nfa out = expr;

	for (const auto e : out.accept())
		out.connect(e, out.start());

	return out;
}

fox_cc::regex_compiler::regex_parser::nfa fox_cc::regex_compiler::regex_parser::nfa_closure_zero_one_expression(const nfa& expr)
{
	nfa out = expr;

	for (const auto e : out.accept())
		out.connect(out.start(), e);

	return out;
}

fox_cc::regex_compiler::regex_parser::nfa fox_cc::regex_compiler::regex_parser::nfa_closure_zero_more_expression(const nfa& expr)
{
	nfa out = expr;

	for (const auto e : out.accept())
	{
		out.connect(out.start(), e);
		out.connect(e, out.start());
	}

	return out;
}

fox_cc::regex_compiler::regex_parser::nfa fox_cc::regex_compiler::regex_parser::compile_nfa(const std::vector<regex_parser_token>& rpn)
{
	std::vector<nfa> nfa_stack;
	nfa_stack.reserve(2);

	for (const auto& token : rpn)
	{
		if (std::holds_alternative<charset>(token))
		{
			nfa_stack.push_back(nfa_charset_expression(std::get<charset>(token)));
		}
		else if (std::holds_alternative<regex_parser_op>(token))
		{
			auto op = std::get<regex_parser_op>(token);

			using enum regex_parser_op;
			switch (op)
			{
			case alternative:
			{
				assert(std::size(nfa_stack) >= 2);
				auto rhs = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				auto lhs = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_union_expression(lhs, rhs));
				break;
			}
			case concatenation:
			{
				assert(std::size(nfa_stack) >= 2);
				auto rhs = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				auto lhs = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_concatenation_expression(lhs, rhs));
				break;
			}
			case closure_one_more:
			{
				assert(std::size(nfa_stack) >= 1);
				auto expr = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_closure_one_more_expression(expr));
				break;
			}
			case closure_zero_more:
			{
				assert(std::size(nfa_stack) >= 1);
				auto expr = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_closure_zero_more_expression(expr));
				break;
			}
			case closure_zero_one:
			{
				assert(std::size(nfa_stack) >= 1);
				auto expr = std::move(nfa_stack.back());
				nfa_stack.pop_back();
				nfa_stack.push_back(nfa_closure_zero_one_expression(expr));
				break;
			}
			default:
				assert(false);
				break;
			}
		}
		else
		{
			assert(false);
		}
	}

	assert(std::size(nfa_stack) == 1);

	return nfa_stack[0];
}

fox_cc::regex_compiler::regex_parser::nfa fox_cc::regex_compiler::regex_parser::compile()
{
	const auto rpn_regex = this->compile_rpn();
	auto nfa = this->compile_nfa(rpn_regex);


	return nfa;
}
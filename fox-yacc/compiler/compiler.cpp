#include "compiler.hpp"

void cmp::compiler::compile()
{
	if (std::empty(ast_.variant_types))
		error("No variant types specified.");

	// Create default type 
	p_default_token_type_ = std::addressof(*types_.insert(default_type).first);

	token_free_list_ = std::vector<uint16_t>(512 - 128);
	std::iota(std::rbegin(token_free_list_), std::rend(token_free_list_), 128); // generate a free list

	// Generate tokens table
	init_char_tokens();
	init_terminal_tokens();
	init_production_tokens();
	optimize_token_table();

	// Productions
	generate_production_table();

	generate_first_sets();
	generate_states();

	// Generate output
	generate_production_list();
	generate_action_table();
	generate_goto_table();
}

cmp::compiler::token* cmp::compiler::token_by_name(std::string_view sv)
{
	auto r = std::ranges::find_if(tokens_,
		[=](const token& t)->bool { return t.name == sv; });

	if (r == std::end(tokens_))
		error("Invalid token name");

	return std::addressof(*r);
}

cmp::token_id_t cmp::compiler::token_id_by_name(std::string_view sv)
{
	const auto r = token_by_name(sv);

	if (r == nullptr)
		return std::numeric_limits<cmp::token_id_t>::max();

	return r->token_id;
}

void cmp::compiler::debug_print()
{
	for(size_t i{}; i < states_.size(); ++i)
	{
		auto& s = states_[i];

		std::cout << "====== " << i << " ======\n";

		for(auto& [prod, follow] : s.productions)
		{
			std::cout << tokens_[prod.name].name << " -> ";

			for (auto stack : prod.stack)
				std::cout << tokens_[stack].name;

			std::cout << ".";

			for (auto p : prod.prod)
				std::cout << tokens_[p].name;

			std::cout << " { ";

			for (auto f : follow)
				if (f != 0)
					std::cout << tokens_[f].name;
				else std::cout << "$";

			std::cout << " }\n";
		}

		for (auto t : s.transitions)
		{
			std::cout << "[ " << tokens_[t.first].name << " => " << t.second << " ]\n";
		}

		std::cout << "\n";
	}
}

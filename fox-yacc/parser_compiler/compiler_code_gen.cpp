#include "compiler.hpp"

void cmp::compiler::generate_production_list()
{
	out_production_list_.reserve(std::size(productions_));

	for(const auto& prod : productions_)
	{
		out_production_list_.push_back({
			.non_terminal = prod.first,
			.count = std::size(prod.second)
		});
	}
}

void cmp::compiler::generate_action_table()
{
	out_action_goto_table_ = action_goto_table_t(std::size(states_), std::size(tokens_));

	for(size_t x = {}; x < std::size(states_); ++x)
	{
		auto& state = states_[x];

		for(const auto& [terminal, k] : state.transitions)
		{
			out_action_goto_table_[x][terminal] = 
			{
				.action = action_type::shift,
				.state = k
			};
		}

		auto get_production_id = [this](const state::production& prod) -> size_t
		{
			for(
				auto [it, end] = productions_.equal_range(prod.name);
				it != end;
				++it
				)
			{
				const production_symbol_t* ptr = nullptr;
				if(std::empty(prod.stack))
					ptr = std::addressof(prod.prod.front());
				else
					ptr = std::addressof(prod.stack.front());
				
				if (ptr == std::data(it->second))
					return std::distance(std::begin(productions_), it);
			}

			assert(false && "Production not found!");
			return 0;
		};

		for (auto& [production, follow] : state.productions)
		{
			if(std::empty(production.prod)) // then reduce
			{
				for(auto a : follow)
				{
					auto& entry = out_action_goto_table_[x][a];

					// check if we have a conflict
					if(entry.state == std::numeric_limits<size_t>::max())
						// no conflict
					{
						entry = {
							.action = action_type::reduce,
							.state = get_production_id(production)
						};
					}
					else
					{
						// TODO:
						assert(false);
					}
				}
			}
		}
	}
}

void cmp::compiler::generate_goto_table()
{

}

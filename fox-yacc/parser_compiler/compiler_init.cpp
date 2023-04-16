#include "compiler.hpp"


void cmp::compiler::init_char_tokens()
{
	// Generate char-types
	for (size_t i = 0; i <= std::numeric_limits<char>::max(); ++i)
	{
		auto name = std::string("'") +
			(std::isalnum(static_cast<int>(i)) ? static_cast<char>(i) : '?') + "'";

		tokens_[i] =
		{
			.type = p_default_token_type_,
			.token_id = static_cast<token_id_t>(i),
			.name = std::move(name),
			.terminal = true
		};
	}
}

void cmp::compiler::init_terminal_tokens()
{
	// Generate terminals
	for (auto& def : ast_.token_definitions)
	{
		// TODO: Deduce assoc

		// Deduce type
		const std::string* type = p_default_token_type_;

		if (static_cast<bool>(def.tag))
		{
			auto r = types_.insert(std::string(def.tag.info->string_value)).first;
			type = std::addressof(*r);
		}

		// Iterate over terminals

#if 0 
		for (auto& term : def.tokens)
		{
			assert_unique_name(term.name);

			size_t pos;

			if (term.number)
			{
				const int v = term.number.info->int_value;
				if (v < 128 || v >= 512)
					error("Manual token id out of range.");

				auto& e = tokens_[v];

				if (e.type != nullptr)
					error("Manual token id already in use.");

				pos = v;
			}
			else
			{
				if (std::empty(token_free_list_))
					error("No token id's available.");

				pos = token_free_list_.back();
				token_free_list_.pop_back();
			}

			auto& e = tokens_[pos];

			e.type = type;
			e.name = term.name.info->string_value;
			e.token_id = pos;
			e.terminal = true;
		}
#endif
	}
}

void cmp::compiler::init_production_tokens()
{
	// Because use is allowed before definition/declaration of the production, 
	// we need to first register all productions

	for (auto& prod : ast_.productions)
	{
		assert_unique_name(prod.name);

		if (std::empty(token_free_list_))
			error("No token id's available.");

		const size_t pos = token_free_list_.back();
		token_free_list_.pop_back();

		auto& e = tokens_[pos];

		const std::string* type = p_default_token_type_;

		if (auto r = ast_.name_type.find(prod.name); r != std::end(ast_.name_type))
		{
			auto l = types_.insert(std::string(r->second.info->string_value)).first;
			type = std::addressof(*l);
		}

		e.type = type;
		e.name = prod.name.info->string_value;
		e.token_id = pos;
		e.terminal = false;
	}
}

void cmp::compiler::optimize_token_table()
{
	auto r = std::find_if(std::rbegin(tokens_), std::rend(tokens_),
		[](const token& t) -> bool {return t.type != nullptr; });

	// TODO: Test
	const auto pos = std::distance(r, std::rend(tokens_));
	tokens_.erase(std::begin(tokens_) + pos, std::end(tokens_));
	tokens_.shrink_to_fit();

	token_free_list_.clear();
	token_free_list_.shrink_to_fit();
}
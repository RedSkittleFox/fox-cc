#pragma once

#include <ranges>
#include "../fox_cc.hpp"

namespace cmp
{
	using token_id_t = uint32_t;
	using terminal_id_t = token_id_t;
	using non_terminal_id_t = token_id_t;

	struct production_symbol_t
	{
		constexpr static size_t no_action = std::numeric_limits<size_t>::max();

		non_terminal_id_t production_id;
		size_t action_id = no_action;

		friend auto operator<=>(const production_symbol_t&, const production_symbol_t&) = default;

		[[nodiscard]] operator const non_terminal_id_t() const noexcept
		{
			return production_id;
		}
	};

	enum class action_type : uint8_t
	{
		shift,
		reduce,
		accept,
		go_to
	};

	struct action_t
	{
		action_type action;
		size_t state = std::numeric_limits<size_t>::max();
	};

	using action_goto_table_t = fox_cc::fixed_2D_array<action_t>;

	struct production_t
	{
		size_t non_terminal;
		size_t count;
	};

	using production_list_t = std::vector<production_t>;
}
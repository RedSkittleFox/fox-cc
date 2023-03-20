#pragma once

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
}
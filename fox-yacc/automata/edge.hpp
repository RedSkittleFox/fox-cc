#pragma once

#include <bitset>

namespace fox_cc
{
	namespace automata
	{
		struct empty_state {};

		template<class Edge>
		struct edge_traits
		{
			static Edge epsilon()
			{
				return std::numeric_limits<Edge>::max();
			}
		};

		template<size_t N>
		struct edge_traits<std::bitset<N>>
		{
			static std::bitset<N> epsilon()
			{
				return std::bitset<N>{}.flip();
			}
		};
	}
}
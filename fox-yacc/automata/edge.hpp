#pragma once

#include <bitset>
#include <ranges>

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

			// Sometimes the single edge might represent multiple transitions, we want to check if those transitions don't overlap,
			// doesn't matter in case of value-based edges
			static bool empty_intersection(const Edge& lhs, const Edge& rhs)
			{
				return lhs != rhs;
			}

			static std::vector<Edge> unique_edges(std::span<const Edge> edges)
			{
				std::set<Edge> set(std::begin(edges), std::end(edges));
				return std::vector<Edge>(std::begin(set), std::end(set));
			}
		};

		template<size_t N>
		struct edge_traits<std::bitset<N>>
		{
			static std::bitset<N> epsilon()
			{
				return std::bitset<N>{}.flip();
			}

			static bool empty_intersection(const std::bitset<N>& lhs, const std::bitset<N>& rhs)
			{
				return (lhs & rhs).none();
			}

			static std::vector<std::bitset<N>> unique_edges(std::span<const std::bitset<N>> edges)
			{
				// characters to bitsets sets
				std::vector<std::set<size_t>> sets(N);

				for(size_t i = 0; i < std::size(edges); ++i)
				{
					for(size_t j = 0; j < std::size(edges[i]); ++j)
					{
						if(edges[i].test(j))
							sets[j].insert(i);
					}
				}

				std::map<std::set<size_t>, std::bitset<N>> edge_groups_to_bitsets;

				for(size_t i = 0; i < std::size(sets); ++i)
				{
					edge_groups_to_bitsets[sets[i]].set(i);
				}

				return edge_groups_to_bitsets
					| std::views::filter([](auto& p) -> bool { return !std::empty(p.first); })
					| std::views::values
					| std::ranges::to<std::vector<std::bitset<N>>>();
			}
		};
	}
}
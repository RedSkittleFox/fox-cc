#pragma once

#include <unordered_map>
#include <vector>
#include <set>

#include <automata/edge.hpp>
#include <automata/nfa.hpp>

namespace fox_cc
{
	namespace automata
	{
		template
		<
			class State,
			class Edge,
			class EdgeTraits = ::fox_cc::automata::edge_traits<Edge>
		>
		class dfa
		{
		public:
			inline static constexpr size_t state_id_npos = std::numeric_limits<size_t>::max();

			class dfa_state
			{
				[[no_unique_address]] State state_;
				std::map<Edge, size_t> next_;

			public:

			};

		private:
			size_t start_ = state_id_npos;
			std::set<size_t> accept_;
			std::vector<dfa_state> states_;

		};
	}
}
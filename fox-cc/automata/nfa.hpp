#pragma once

#include <map>
#include <vector>
#include <cassert>
#include <ranges>
#include <set>
#include <unordered_map>

#include <automata/state_machine.hpp>

namespace fox_cc
{
	namespace automata
	{
		template
		<
			class Value,
			class Reduce,
			class Edge,
			class EdgeTraits = ::fox_cc::automata::edge_traits<Edge>
		>
		using nfa = state_machine<Value, Reduce, Edge, EdgeTraits, false, true>;

		
	}
}
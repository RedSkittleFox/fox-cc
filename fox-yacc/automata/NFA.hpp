#pragma once

#include <map>
#include <vector>
#include <cassert>
#include <ranges>
#include <set>
#include <unordered_map>

#include <automata/edge.hpp>

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
		class nfa
		{
		public:
			inline static constexpr size_t state_id_npos = std::numeric_limits<size_t>::max();
		private:
			class nfa_state
			{
				template<class, class, class>
				friend class nfa;

				[[no_unique_address]] State value_;

				std::unordered_multimap<Edge, size_t> previous_; // backwards edges
				std::unordered_multimap<Edge, size_t> next_; // follow edges

				size_t state_id_ = state_id_npos;

			public:
				[[nodiscard]] State& value() noexcept
				{
					return value_;
				}

				[[nodiscard]] const State& value() const noexcept
				{
					return value_;
				}

				[[nodiscard]] auto previous() -> const decltype(previous_)&
				{
					return previous_;
				}

				[[nodiscard]] auto next() -> const decltype(next_)&
				{
					return previous_;
				}

				[[nodiscard]] auto state_id() const noexcept
				{
					return state_id_;
				}
			};

			size_t start_ = state_id_npos;
			std::set<size_t> accept_;
			std::vector<nfa_state> states_;

		private:
			void assert_state_id_in_range(size_t id)
			{
				assert(id < std::size(states_));
			}

		public:
			nfa() = default;
			nfa(const nfa&) = default;
			nfa(nfa&&) noexcept = default;
			nfa& operator=(const nfa&) = default;
			nfa& operator=(nfa&&) noexcept = default;
			~nfa() noexcept = default;

		public:
			auto begin() -> decltype(auto)
			{
				return std::begin(states_);
			}

			auto begin() const -> decltype(auto)
			{
				return std::begin(states_);
			}

			auto cbegin() const -> decltype(auto)
			{
				return std::cbegin(states_);
			}

			auto end() -> decltype(auto)
			{
				return std::end(states_);
			}

			auto end() const -> decltype(auto)
			{
				return std::end(states_);
			}

			auto cend() const -> decltype(auto)
			{
				return std::cend(states_);
			}

		public:
			[[nodiscard]] size_t size() const noexcept
			{
				return std::size(states_);
			}

			[[nodiscard]] size_t empty() const noexcept
			{
				return std::empty(states_);
			}

			[[nodiscard]] nfa_state& operator[](size_t idx) noexcept
			{
				return states_[idx];
			}

			[[nodiscard]] const nfa_state& operator[](size_t idx) const noexcept
			{
				return states_[idx];
			}

			[[nodiscard]] nfa_state& at(size_t idx) noexcept
			{
				return states_.at(idx);
			}

			[[nodiscard]] const nfa_state& at(size_t idx) const noexcept
			{
				return states_.at(idx);
			}

		public:
			[[nodiscard]] size_t& start() noexcept
			{
				return start_;
			}

			[[nodiscard]] size_t start() const noexcept
			{
				return start_;
			}

			[[nodiscard]] std::set<size_t>& accept() noexcept
			{
				return accept_;
			}

			[[nodiscard]] const std::set<size_t>& accept() const noexcept
			{
				return accept_;
			}

		public:
			/**
			 * \brief Inserts new state into NFA, returns state ID
			 * \param value State's internal value
			 * \return State ID
			 */
			[[nodiscard]] size_t insert(const State& value = State())
			{
				states_.emplace_back();
				states_.back().value_ = value;
				return states_.back().state_id_ = std::size(states_) - 1;
			}

			/**
			 * \brief merges two NFAs
			 * \param NFA NFA we want to merge
			 * \return Mapping of old node IDs to new node IDs
			 */
			[[nodiscard]] std::vector<size_t> insert(const nfa& NFA)
			{
				std::vector<size_t> out(std::size(NFA.states_));

				const size_t offset = std::size(states_);

				for(const auto& s : NFA.states_)
				{
					states_.push_back(s);
					states_.back().state_id_ += offset;

					for (auto& c : states_.back().previous_ | std::views::values)
						c += offset;

					for (auto& c : states_.back().next_ | std::views::values)
						c += offset;

					out[s.state_id_] = states_.back().state_id_;
				}

				return out;
			}

			void connect(size_t from_id, size_t to_id, const Edge& edge = EdgeTraits::epsilon())
			{
				assert_state_id_in_range(from_id);
				assert_state_id_in_range(to_id);

				auto& from = this->operator[](from_id);
				auto& to = this->operator[](to_id);

				// avoid duplicates
				for(auto [start, end] = from.next_.equal_range(edge); start != end; ++start)
				{
					if (start->second == to_id)
						return;
				}

				from.next_.insert(std::make_pair(edge, to_id));
				to.previous_.insert(std::make_pair(edge, from_id));
			}

			void disconnect(size_t from_id, size_t to_id)
			{
				assert_state_id_in_range(from_id);
				assert_state_id_in_range(to_id);

				auto& from = this->operator[](from_id);
				auto& to = this->operator[](to_id);

				std::erase_if(from.next_, [=](const auto& v) -> bool
					{
						return v.second == to_id;
					});

				std::erase_if(to.previous_, [=](const auto& v) -> bool
					{
						return v.second == from_id;
					});
			}

			void move_state(size_t from_id, size_t to_id)
			{
				assert_state_id_in_range(from_id);
				assert_state_id_in_range(to_id);

				if (from_id == to_id)
					return;

				auto& from = this->operator[](from_id);
				auto& to = this->operator[](to_id);

				erase(to, false);

				auto from_id_predicate = [=](auto v) -> bool {return v == from_id; };

				for (auto& id : from.next_ | std::views::values)
				{
					nfa_state& next = this->operator[](id);

					for(auto& iid : next.previous_ | std::views::values | std::views::filter(from_id_predicate))
						iid = to;
				}

				for (auto& id : from.previous_ | std::views::values)
				{
					nfa_state& previous = this->operator[](id);

					for (auto& iid : previous.next_ | std::views::values | std::views::filter(from_id_predicate))
						iid = to;
				}

				to = std::move(from);
				to.state_id_ = to_id;

				if (from_id == std::size(states_) - 1)
					states_.pop_back();
			}

			void erase(size_t state_id, bool optimize = true)
			{
				assert_state_id_in_range(state_id);

				auto& state = this->operator[](state_id);

				// void all connections
				while(!std::empty(state.next_))
				{
					disconnect(state_id, state.next_.begin()->second);
				}

				while (!std::empty(state.previous_))
				{
					disconnect(state.previous_.begin()->second, state_id);
				}

				if(optimize && std::size(states_) >= 2)
				{
					move_state(std::size(states_) - 1, state_id);
				}
			}
		};
	}
}
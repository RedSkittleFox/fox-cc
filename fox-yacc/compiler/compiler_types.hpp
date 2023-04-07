#pragma once

#include <ranges>

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

	template<class T>
	class fixed_2D_array
	{
		size_t width_;
		size_t height_;

		T* data_;

	public:
		void clear() noexcept
		{
			delete[] data_;
			width_ = 0;
			height_ = 0;
		}

	public:
		fixed_2D_array() : width_(0), height_(0), data_(nullptr) {}

		fixed_2D_array(size_t width, size_t height, T default_val = T())
			: width_(width), height_(height), data_(new T[width * height])
		{
			for (auto s = std::ranges::subrange(data_, data_ + width * height); auto & i : s)
				i = default_val;
		}

		fixed_2D_array(const fixed_2D_array& other) = delete;
		fixed_2D_array& operator=(const fixed_2D_array& other) = delete;

	public:
		fixed_2D_array(fixed_2D_array&& other) noexcept
			:
			width_(std::exchange(other.width_, {})),
			height_(std::exchange(other.height_, {})),
			data_(std::exchange(other.data_, {}))
		{}

		fixed_2D_array& operator=(fixed_2D_array&& other) noexcept
		{
			this->clear();

			width_ = std::exchange(other.width_, {});
			height_ = std::exchange(other.height_, {});
			data_ = std::exchange(other.data_, {});
			return *this;
		}

		~fixed_2D_array() noexcept
		{
			this->clear();
		}

		T* operator[](size_t idx) noexcept
		{
			assert(idx * height_ < width_ * height_);

			return std::addressof(data_[idx * height_]);
		}

		const T* operator[](size_t idx) const noexcept
		{
			assert(idx * height_ < width_ * height_);

			return std::addressof(data_[idx * height_]);
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

	using action_goto_table_t = fixed_2D_array<action_t>;

	struct production_t
	{
		size_t non_terminal;
		size_t count;
	};

	using production_list_t = std::vector<production_t>;
}
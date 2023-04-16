#pragma once

#include <vector>
#include <string>
#include <cassert>
#include <bitset>

namespace fox_cc
{
	using charset = std::bitset<128>;

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

	struct lexer
	{
		using token_id = size_t;

		std::vector<std::string> tokens; // maps token-id to name
		fixed_2D_array<size_t> transition_diagram;
		std::vector<token_id> reduce_map; // maps state_id to token_id
	};

	class parser
	{
		
	};
}
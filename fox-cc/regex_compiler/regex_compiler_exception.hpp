#pragma once

#include <exception>
#include <string>

namespace fox_cc
{
	namespace regex_compiler
	{
		class regex_exception : public std::exception
		{
			std::string regex_;
			std::string message_;
			size_t token_;

		public:
			regex_exception() = delete;

			regex_exception(std::string regex, std::string message, size_t token) noexcept
				: regex_(std::move(regex)), message_(std::move(message)), token_(token) {}

			regex_exception(const regex_exception&) noexcept = default;

			regex_exception(regex_exception&&) noexcept = default;

			regex_exception& operator=(const regex_exception&) noexcept = default;

			regex_exception& operator=(regex_exception&&) noexcept = default;

			~regex_exception() noexcept override = default;

		public:
			[[nodiscard]] const std::string& regex() const noexcept
			{
				return regex_;
			}

			[[nodiscard]] const std::string& message() const noexcept
			{
				return message_;
			}

			[[nodiscard]] size_t token() const noexcept
			{
				return token_;
			}

			[[nodiscard]] const char* what() const noexcept override
			{
				return message_.c_str();
			}
		};
	}
}
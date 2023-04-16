#include <regex_compiler/regex_compiler.hpp>

void fox_cc::regex_compiler::regex_parser::throw_error(const char* error_string) const noexcept(false)
{
	throw fox_cc::regex_compiler::regex_exception(static_cast<std::string>(this->str_), error_string, this->lexer_current_char_pos_);
}

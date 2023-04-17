#include <variant>
#include <memory>
#include <list>
#include <cassert>
#include <functional>

#include <regex_compiler/regex_compiler.hpp>
#include <automata/dfa.hpp>

#include "lex_compiler.hpp"

using namespace fox_cc;

fox_cc::lex_compiler::regex_productions fox_cc::lex_compiler::generate_production(std::string_view regex)
{
	regex_compiler::regex_parser rgx(regex);
	auto nfa = rgx.compile();

	auto dfa = automata::construct_dfa(nfa);

	return {};
}

void fox_cc::lex_compiler::populate_first_sets(regex_productions& prods)
{

}
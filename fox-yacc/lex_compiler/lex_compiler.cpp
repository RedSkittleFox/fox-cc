#include <lex_compiler/lex_compiler.hpp>
#include <regex_compiler/regex_compiler.hpp>

fox_cc::lex_compiler::lex_compiler(const prs::yacc_ast& ast)
	: ast_(ast)
{
	this->result_.terminals.emplace_back(
		"null-terminator",
		"",
		lex_compiler_result::associativity::token
	);

	automata::nfa<automata::empty_state, size_t, charset> nfa;
	// Create start state for the nfa
	nfa.start() = nfa.insert();

	std::map<std::string, size_t, std::less<>> token_id;

	for (size_t i = 0; i < std::size(ast_.token_definitions); ++i)
	{
		auto& token = ast_.token_definitions[i];

		auto r = token_id.find(token.name.info->string_value);
		if (r != std::end(token_id))
		{
			error("Token already defined."); // TODO: Better error reporting system
		}

		lex_compiler_result::associativity assoc;

		if(token.rword.info->string_value == "token")
		{
			assoc = lex_compiler_result::associativity::token;
		}
		else if(token.rword.info->string_value == "left")
		{
			assoc = lex_compiler_result::associativity::left;
		}
		else if (token.rword.info->string_value == "right")
		{
			assoc = lex_compiler_result::associativity::right;
		}
		else if (token.rword.info->string_value == "nonassoc")
		{
			assoc = lex_compiler_result::associativity::nonassoc;
		}
		else
		{
			error("Unknown token assoc."); // TODO: Better reporting
		}


		this->result_.terminals.emplace_back(
			std::string(token.name.info->string_value),
			token.tag.info == nullptr ? "" : std::string(token.tag.info->string_value),
			assoc
		);

		// Register the token
		token_id[std::string(token.name.info->string_value)] = i;

		try
		{
			regex_compiler::regex_parser regex(token.regex.info->string_value);
			auto token_nfa = regex.compile();

			for(auto accept_state : token_nfa.accept())
			{
				token_nfa[accept_state].reduce() = i;
			}

			assert(token_nfa.start() != token_nfa.state_id_npos);

			// Merge nfa
			auto map = nfa.insert(token_nfa);
			nfa.connect(nfa.start(), map[token_nfa.start()]);
		}
		catch(const regex_compiler::regex_exception& e)
		{
			error("Failed to compile regex."); // TODO: Why
		}
	}

	// Construct DFA
	auto reduce_conflict_resolver = [&, this](size_t lhs, size_t rhs) -> size_t
	{
		this->warning("Lexer reduce conflict. Defaulting to symbol definition order.");
		return std::min(lhs, rhs);
	};

	auto merge_conflict_resolver = [&](automata::empty_state lhs, automata::empty_state rhs) {return lhs; };

	this->result_.dfa = automata::construct_dfa(nfa,
		reduce_conflict_resolver,
		merge_conflict_resolver
	);
}

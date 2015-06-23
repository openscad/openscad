#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"
#include <string>

class Lex 
{
	public:
	lexertl::state_machine sm;
	lexertl::rules rules_;
	std::string token;

	Lex();
	void rules();
	void defineRules(std::string words[], int, int);
	void lex_results(const std::string source);
};

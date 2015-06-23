#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"
#include <string>

class Lex 
{
	public:
	lexertl::state_machine sm;
	lexertl::rules rules_;

	Lex();
	void rules();
	void defineRules(std::string words[], int, int);
};

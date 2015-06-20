#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"

class Lex 
{
	public:
	lexertl::state_machine sm;
	lexertl::rules rules_;
	std::string token;

	Lex();	
};

#include "lex.h"

Lex::Lex()
{
	rules_.push_state("COMMENT");
	rules_.push("[0-9]+", 8);
	rules_.push("[a-zA-Z0-9_]+", 9);
	rules_.push(".", 3);
	rules_.push("\n",3);
	rules_.push("INITIAL", "\"/*\"", "COMMENT");
	rules_.push("COMMENT", "[^*]+|.", ".");
	rules_.push("[/][/].*$", 1);
	
}


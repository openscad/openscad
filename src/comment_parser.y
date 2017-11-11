%glr-parser

%{
	#include <sstream>
	#include <string.h>
	#include "Assignment.h"
	#include "expression.h"
	#include "printutils.h"
	#include "value.h" 
	#include "comment.h" 
	void yyerror(const char *);
	int comment_lexerlex(void);
	int comment_parserlex(void);
	extern void comment_lexer_scan_string ( const char *str );
	Expression *params;
%}
%union {
	char *text;
	char ch;
	double num;
	class Vector *vec;
	class Expression *expr;
};


%token<num> NUM
%token<text> WORD

%type <text> word
%type <expr> expr
%type <expr> num
%type <expr> value
%type <vec> values
%type <expr> wordexpr
%type <expr> params
%type <vec> labeled_vectors

%%


params:
	expr
		{
			$$ = $1;
			params = $$;
		}
	;

expr: 
	'[' values ',' value ']'
	{
		$$ = $2;
		$2->push_back($4);
	}
	| '[' num ':' num']'
	{
		$$ = new Range($2, $4, Location::NONE);
	}
	| '[' num ':' num ':' num ']'
	{
		$$ = new Range($2, $4, $6, Location::NONE);
	}
	;

num:
	NUM
	{
		$$ = new Literal(ValuePtr($1));
	}
	;

value:
	wordexpr
	{
		$$ = $1;
	}
	|labeled_vectors
	{
		$$ = $1;
	}
	;

values:
	value
	{
		$$ = new Vector(Location::NONE);
		$$->push_back($1);
	}
	|values ',' value
	{
		$$ = $1;
		$$->push_back($3);
	}
	;

labeled_vectors: 
	wordexpr ':' wordexpr
	{
		$$ = new Vector(Location::NONE);
		$$->push_back($1);
		$$->push_back($3);
	}
	;

wordexpr:
	 word
	{
		$$ = new Literal(ValuePtr(std::string($1)));
		free($1);
	}
	;

word:   
	WORD
	{
		$$=$1;    
	}
	| NUM
	{
		std::ostringstream strs;
		strs << $1;
		$$ = strdup(strs.str().c_str());
	}
	| word WORD
	{
		std::ostringstream strs;
		strs << $1 << " " << $2;
		$$ = strdup(strs.str().c_str());
	}
	
%%

int comment_parserlex(void)
{
  return comment_lexerlex();
}

void yyerror(const char * /*msg*/) {
	PRINTD("ERROR IN PARAMETER: Parser error in comments of file \n "); 
	params = NULL;
}

shared_ptr<Expression> CommentParser::parser(const char *text)
{
  comment_lexer_scan_string(text);
  int parserretval = comment_parserparse();
  if (parserretval != 0) return NULL;
  return shared_ptr<Expression>(params);
}

%expect 3

%{
    #include <sstream>
    #include <string>
    #include <memory>
    #include "core/Expression.h"
    #include "utils/printutils.h"
    #include "core/customizer/CommentParser.h"
    #ifdef _MSC_VER
    #define strdup _strdup
    #endif

    void yyerror(const char *);
    int comment_lexerlex(void);
    int comment_parserlex(void);
    extern void comment_scan_string ( const char *str );
    std::shared_ptr<Expression> params;
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
            params = std::shared_ptr<Expression>($$);
        }
    ;

expr: 
    '[' values ']'
    {
        $$ = $2;
    }
    | num
    {
        $$ = $1;
    }
    | wordexpr
    {
        $$ = $1;
    }
    | '[' num ':' num ']'
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
        $$ = new Literal($1);
    }
    ;

value:
    labeled_vectors
    {
        $$ = $1;
    }
    |num
    {
        $$ = $1;
    }
    |wordexpr
    {
        $$ = $1;
    }
    ;

values:
    value
    {
        $$ = new Vector(Location::NONE);
        $$->emplace_back($1);
    }
    |values ',' value
    {
        $$ = $1;
        $$->emplace_back($3);
    }
    ;

labeled_vectors: 
    num ':' num
    {
        $$ = new Vector(Location::NONE);
        $$->emplace_back($1);
        $$->emplace_back($3);
    }
    |num ':' wordexpr
    {
        $$ = new Vector(Location::NONE);
        $$->emplace_back($1);
        $$->emplace_back($3);
    }
    |wordexpr ':' num
    {
        $$ = new Vector(Location::NONE);
        $$->emplace_back($1);
        $$->emplace_back($3);
    }
    |wordexpr ':' wordexpr
    {
        $$ = new Vector(Location::NONE);
        $$->emplace_back($1);
        $$->emplace_back($3);
    }
    ;

wordexpr:
    word
    {
        $$ = new Literal(std::string($1));
        free($1);
    }
    ;

word:
    WORD
    {
        $$=$1;    
    }
    | word NUM
    {
        std::ostringstream strs;
        strs << $1 << " " << $2;
        $$ = strdup(strs.str().c_str());
        free($1);
    }
    | NUM word
    {
        std::ostringstream strs;
        strs << $1 << " " << $2;
        $$ = strdup(strs.str().c_str());
        free($2);
    }
    | word WORD
    {
        std::ostringstream strs;
        strs << $1 << " " << $2;
        $$ = strdup(strs.str().c_str());
        free($1);
        free($2);
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

std::shared_ptr<Expression> CommentParser::parser(const char *text)
{
  comment_scan_string(text);
  int parserretval = comment_parserparse();
  if (parserretval != 0) return nullptr;
  return params;
}

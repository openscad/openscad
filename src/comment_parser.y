%{
    #include <sstream>
    #include <string.h>
    #include "Assignment.h"
    #include "expression.h"
    #include "printutils.h"
    #include "value.h" 
    #include "comment.h" 
    void yyerror(char *);
    int yylex(void);
    extern void yy_scan_string ( const char *str );
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
%type <expr> params
%type <vec> vector_expr
%type <vec> labled_vector

%%


params:
          expr
            {
                $$ = $1;
                params = $$;
            }			
            ;
            
expr: 
         NUM 
            {
                $$ = new Literal(ValuePtr($1));
                
            }
        | word
            {
                $$ = new Literal(ValuePtr(std::string($1)));
                free($1);
            }
        | '[' optional_commas ']'
            {
                $$ = new Literal(ValuePtr(Value::VectorType()));
            }
        | '[' vector_expr optional_commas ']'
            {
                $$ = $2;
            }            
        | '[' expr ':' expr ']'
            {
                $$ = new Range($2, $4,Location::NONE);
            }
        | '[' expr ':' expr ':' expr ']'
            {
                $$ = new Range($2, $4, $6,Location::NONE);
            }
        | labled_vector { $$=$1;}
        ;
                
labled_vector: 
        expr ':' expr {
            $$ = new Vector(Location::NONE);
            $$->push_back($1);
            $$->push_back($3);
        }
		;

optional_commas:
          ',' optional_commas
        | /* empty */
        ;
       
vector_expr:
          expr
            {
                $$ = new Vector(Location::NONE);
                $$->push_back($1);
            }
            | vector_expr ',' optional_commas expr
            {
                $$ = $1;
                $$->push_back($4);
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
    }
    | NUM word
    {
        std::ostringstream strs;
        strs << $1 << " " << $2;
        $$ = strdup(strs.str().c_str());
    }
    | word WORD
    {
        std::ostringstream strs;
        strs << $1 << " " << $2;
        $$ = strdup(strs.str().c_str());
    }
%%

void yyerror(char *msg) {
    PRINTD("ERROR IN PARAMETER: Parser error in comments of file \n "); 
    params = NULL;
}

shared_ptr<Expression> CommentParser::parser(const char *text)
{
  yy_scan_string(text);
  int parserretval = yyparse();
  if (parserretval != 0) return NULL;
  return shared_ptr<Expression>(params);
}

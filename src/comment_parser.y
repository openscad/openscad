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
    AssignmentList *argument;
      extern void yy_scan_string ( const char *str );
%}
%union {
    char *text;
    char ch;
    double num;
    class Expression *expr;
    class Vector *vec;
    Assignment *arg;
    AssignmentList *args;
    
};


%token<num> NUM

%token<text> WORD
%type <text> word
%type <expr> expr
%type <args> arguments_call
%type <arg> argument_call
%type <vec> vector_expr
%type <vec> labled_vector

%%


arguments_call:
        argument_call
            {
                $$ = new AssignmentList();
                $$->push_back(*$1);
                 argument=$$;
                delete $1;
            }
        ;


argument_call:
          expr
            {
                $$ = new Assignment("", shared_ptr<Expression>($1));
            }			
            ;
            
expr		: 
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
        std::string a;
        a=$1;
        a+=" ";
        double dbl=$2;
        std::ostringstream strs;
        strs<<dbl;
        a=a+strs.str();
        $$=strdup(a.c_str());
    }
    | NUM word
    {
        double dbl=$1;
        std::ostringstream strs;
        strs<<dbl;
        std::string a=" ";
        a+=$2;
        a=strs.str()+a;
        $$=strdup(a.c_str());
    }
    | word WORD
    {
        std::string a;
        a=$1;
        a+=" ";
        a+=$2;
        $$=strdup(a.c_str());
    }
%%

void yyerror(char *msg) {
    PRINTD("ERROR IN PARAMETER: Parser error in comments of file \n "); 
    argument=NULL;
}

AssignmentList *CommentParser::parser(const char *text)
{
  yy_scan_string(text);
  int parserretval = yyparse();
  if (parserretval != 0) return NULL;
  return argument;
}

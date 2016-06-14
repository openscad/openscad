%{
    #include<string.h>
    #include<iostream>
    using namespace std;
    #include "typedefs.h"
    #include "module.h"
    #include "expression.h"
    #include "value.h" 
    void yyerror(char *);
    int yylex(void);
    AssignmentList *argument;
%}
%union {
    char *text;
    char ch;
    class Expression *expr;
    Assignment *arg;
    AssignmentList *args;
    
};

%token<text> NUM
%token<text> WORD
%type <expr> expr
%type <args> arguments_call
%type <args> input
%type <arg> argument_call
%type <expr> vector_expr

%%

input:
    arguments_call
            {
                argument=$1;
            }
            ;
arguments_call:
          /* empty */
            {
                $$ = new AssignmentList();
            }
        | argument_call
            {
                $$ = new AssignmentList();
                $$->push_back(*$1);
                delete $1;
            }
        | arguments_call ',' argument_call
            {
                $$ = $1;
                $$->push_back(*$3);
                delete $3;
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
                $$ = new ExpressionConst(ValuePtr($1));
            }
        | WORD
            {
                $$ = new ExpressionConst(ValuePtr(std::string($1)));
                free($1);
            }
        | '[' optional_commas ']'
            {
                $$ = new ExpressionConst(ValuePtr(Value::VectorType()));
            }
        | '[' vector_expr optional_commas ']'
            {
                $$ = $2;
            }
        | '[' expr ':' expr ']'
            {
                $$ = new ExpressionRange($2, $4);
            }
        | '[' expr ':' expr ':' expr ']'
            {
                $$ = new ExpressionRange($2, $4, $6);
            }
		;

optional_commas:
          ',' optional_commas
        | /* empty */
        ;
       
vector_expr:
          expr
            {
                $$ = new ExpressionVector($1);
            }
            ;		
%%

void yyerror(char *s) {
    cout<<s<<endl;   
}

int parse(void) {
   yyparse();;
    return 0;
}

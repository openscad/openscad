%{
    #include<string.h>
    #include<iostream>
    #include<stdio.h>
    using namespace std;
    #include "typedefs.h"
    #include "module.h"
    #include "expression.h"
    #include "value.h" 
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
    Assignment *arg;
    AssignmentList *args;
    
};


%token<num> NUM
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
        | arguments_call ',' optional_commas argument_call
            {
                $$ = $1;
                $$->push_back(*$4);
                delete $4;
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
            |  expr ':' expr 
            {   
                $$ = new ExpressionVector($1);
                $$->children.push_back($3);
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
            | vector_expr ',' optional_commas expr
            {
                $$ = $1;
                $$->children.push_back($4);
            }
            ;		
%%

void yyerror(char *s) {
    cout<<s<<endl;   
}

AssignmentList * parser(const char *text) {

    yy_scan_string(text);
    int parserretval = yyparse();
    if (parserretval != 0) return NULL;
    return argument;
    
}

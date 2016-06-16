%{
    #include<iostream>
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
%type <arg> argument_call
%type <expr> vector_expr

%%


arguments_call:
          /* empty */
            {
                $$ = new AssignmentList();
            }
        | argument_call
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

void yyerror(char *msg) {
    cout<<msg<<endl;   
}

AssignmentList * parser(const char *text) {

    yy_scan_string(text);
    int parserretval = yyparse();
    return argument;
    
}

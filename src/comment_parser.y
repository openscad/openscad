%{
    #include<iostream>
    #include<string.h>
    using namespace std;
    #include "Assignment.h"
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
    | word WORD
    {
        string a;
        a=$1;
        a+=" ";
        a+=$2;
        $$=strdup(a.c_str());
    }
%%

void yyerror(char *msg) {
    cout<<msg<<endl;   
}

AssignmentList * parser(const char *text) {

    yy_scan_string(text);
    int parserretval = yyparse();
    if (parserretval != 0) return NULL;
    return argument;
    
}

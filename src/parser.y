/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

%expect 1 /* Expect 1 shift/reduce conflict for ifelse_statement - "dangling else problem" */

%{

#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "typedefs.h"
#include "module.h"
#include "expression.h"
#include "value.h"
#include "function.h"
#include "printutils.h"
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
#define foreach BOOST_FOREACH

#include "boosty.h"

int parser_error_pos = -1;

int parserlex(void);
void yyerror(char const *s);

int lexerget_lineno(void);
int lexerlex_destroy(void);
int lexerlex(void);

std::stack<LocalScope *> scope_stack;
FileModule *rootmodule;

extern void lexerdestroy();
extern FILE *lexerin;
extern const char *parser_input_buffer;
const char *parser_input_buffer;
std::string parser_source_path;

%}

%union {
  char *text;
  double number;
  class Value *value;
  class Expression *expr;
  class ModuleInstantiation *inst;
  class IfElseModuleInstantiation *ifelse;
  Assignment *arg;
  AssignmentList *args;
}

%token TOK_ERROR

%token TOK_MODULE
%token TOK_FUNCTION
%token TOK_IF
%token TOK_ELSE
%token TOK_FOR
%token TOK_LET

%token <text> TOK_ID
%token <text> TOK_STRING
%token <text> TOK_USE
%token <number> TOK_NUMBER

%token TOK_TRUE
%token TOK_FALSE
%token TOK_UNDEF

%token LE GE EQ NE AND OR

%right LET

%right '?' ':'

%left OR
%left AND

%left '<' LE GE '>'
%left EQ NE

%left '!' '+' '-'
%left '*' '/' '%'
%left '[' ']'
%left '.'

%type <expr> expr
%type <expr> vector_expr
%type <expr> list_comprehension_elements
%type <expr> list_comprehension_elements_or_expr

%type <inst> module_instantiation
%type <ifelse> if_statement
%type <ifelse> ifelse_statement
%type <inst> single_module_instantiation

%type <args> arguments_call
%type <args> arguments_decl

%type <arg> argument_call
%type <arg> argument_decl
%type <text> module_id

%debug

%%

input:    /* empty */
        | TOK_USE
            { rootmodule->registerUse(std::string($1)); }
          input
        | statement input
        ;

statement:
          ';'
        | '{' inner_input '}'
        | module_instantiation
            {
                if ($1) scope_stack.top()->addChild($1);
            }
        | assignment
        | TOK_MODULE TOK_ID '(' arguments_decl optional_commas ')'
            {
                Module *newmodule = new Module();
                newmodule->definition_arguments = *$4;
                scope_stack.top()->modules[$2] = newmodule;
                scope_stack.push(&newmodule->scope);
                free($2);
                delete $4;
            }
          statement
            {
                scope_stack.pop();
            }
        | TOK_FUNCTION TOK_ID '(' arguments_decl optional_commas ')' '=' expr
            {
                Function *func = Function::create($2, *$4, $8);
                scope_stack.top()->functions[$2] = func;
                free($2);
                delete $4;
            }
          ';'
        ;

inner_input:
          /* empty */
        | statement inner_input
        ;

assignment:
          TOK_ID '=' expr ';'
            {
                bool found = false;
                foreach (Assignment& iter, scope_stack.top()->assignments) {
                    if (iter.first == $1) {
                        iter.second = boost::shared_ptr<Expression>($3);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    scope_stack.top()->assignments.push_back(Assignment($1, boost::shared_ptr<Expression>($3)));
                }
                free($1);
            }
        ;

module_instantiation:
          '!' module_instantiation
            {
                $$ = $2;
                if ($$) $$->tag_root = true;
            }
        | '#' module_instantiation
            {
                $$ = $2;
                if ($$) $$->tag_highlight = true;
            }
        | '%' module_instantiation
            {
                $$ = $2;
                if ($$) $$->tag_background = true;
            }
        | '*' module_instantiation
            {
                delete $2;
                $$ = NULL;
            }
        | single_module_instantiation
            {
                $<inst>$ = $1;
                scope_stack.push(&$1->scope);
            }
          child_statement
            {
                scope_stack.pop();
                $$ = $<inst>2;
            }
        | ifelse_statement
            {
                $$ = $1;
            }
        ;

ifelse_statement:
          if_statement
            {
                $$ = $1;
            }
        | if_statement TOK_ELSE
            {
                scope_stack.push(&$1->else_scope);
            }
          child_statement
            {
                scope_stack.pop();
                $$ = $1;
            }
        ;

if_statement:
          TOK_IF '(' expr ')'
            {
                $<ifelse>$ = new IfElseModuleInstantiation();
                $<ifelse>$->arguments.push_back(Assignment("", boost::shared_ptr<Expression>($3)));
                $<ifelse>$->setPath(parser_source_path);
                scope_stack.push(&$<ifelse>$->scope);
            }
          child_statement
            {
                scope_stack.pop();
                $$ = $<ifelse>5;
            }
        ;

child_statements:
          /* empty */
        | child_statements child_statement
        | child_statements assignment
        ;

child_statement:
          ';'
        | '{' child_statements '}'
        | module_instantiation
            {
                if ($1) scope_stack.top()->addChild($1);
            }
        ;

// "for" is a valid module identifier
module_id:
          TOK_ID  { $$ = $1; }
        | TOK_FOR { $$ = strdup("for"); }
        ;

single_module_instantiation:
          module_id '(' arguments_call ')'
            {
                $$ = new ModuleInstantiation($1);
                $$->arguments = *$3;
                $$->setPath(parser_source_path);
                free($1);
                delete $3;
            }
        ;

expr:
          TOK_TRUE
            {
                $$ = new ExpressionConst(ValuePtr(true));
            }
        | TOK_FALSE
            {
                $$ = new ExpressionConst(ValuePtr(false));
            }
        | TOK_UNDEF
            {
                $$ = new ExpressionConst(ValuePtr::undefined);
            }
        | TOK_ID
            {
                $$ = new ExpressionLookup($1);
                free($1);
            }
        | expr '.' TOK_ID
            {
              $$ = new ExpressionMember($1, $3);
                free($3);
            }
        | TOK_STRING
            {
                $$ = new ExpressionConst(ValuePtr(std::string($1)));
                free($1);
            }
        | TOK_NUMBER
            {
                $$ = new ExpressionConst(ValuePtr($1));
            }
        | TOK_LET '(' arguments_call ')' expr %prec LET
            {
              $$ = new ExpressionLet(*$3, $5);
                delete $3;
            }
        | '[' expr ':' expr ']'
            {
                $$ = new ExpressionRange($2, $4);
            }
        | '[' expr ':' expr ':' expr ']'
            {
                $$ = new ExpressionRange($2, $4, $6);
            }
        | '[' list_comprehension_elements ']'
            {
                $$ = new ExpressionLcExpression($2);
            }
        | '[' optional_commas ']'
            {
                $$ = new ExpressionConst(ValuePtr(Value::VectorType()));
            }
        | '[' vector_expr optional_commas ']'
            {
                $$ = $2;
            }
        | expr '*' expr
            {
                $$ = new ExpressionMultiply($1, $3);
            }
        | expr '/' expr
            {
                $$ = new ExpressionDivision($1, $3);
            }
        | expr '%' expr
            {
                $$ = new ExpressionModulo($1, $3);
            }
        | expr '+' expr
            {
                $$ = new ExpressionPlus($1, $3);
            }
        | expr '-' expr
            {
                $$ = new ExpressionMinus($1, $3);
            }
        | expr '<' expr
            {
                $$ = new ExpressionLess($1, $3);
            }
        | expr LE expr
            {
                $$ = new ExpressionLessOrEqual($1, $3);
            }
        | expr EQ expr
            {
                $$ = new ExpressionEqual($1, $3);
            }
        | expr NE expr
            {
                $$ = new ExpressionNotEqual($1, $3);
            }
        | expr GE expr
            {
                $$ = new ExpressionGreaterOrEqual($1, $3);
            }
        | expr '>' expr
            {
                $$ = new ExpressionGreater($1, $3);
            }
        | expr AND expr
            {
                $$ = new ExpressionLogicalAnd($1, $3);
            }
        | expr OR expr
            {
                $$ = new ExpressionLogicalOr($1, $3);
            }
        | '+' expr
            {
                $$ = $2;
            }
        | '-' expr
            {
                $$ = new ExpressionInvert($2);
            }
        | '!' expr
            {
                $$ = new ExpressionNot($2);
            }
        | '(' expr ')'
            {
                $$ = $2;
            }
        | expr '?' expr ':' expr
            {
                $$ = new ExpressionTernary($1, $3, $5);
            }
        | expr '[' expr ']'
            {
                $$ = new ExpressionArrayLookup($1, $3);
            }
        | TOK_ID '(' arguments_call ')'
            {
              $$ = new ExpressionFunctionCall($1, *$3);
                free($1);
                delete $3;
            }
        ;

list_comprehension_elements:
          /* The last set element may not be a "let" (as that would instead
             be parsed as an expression) */
          TOK_LET '(' arguments_call ')' list_comprehension_elements
            {
              $$ = new ExpressionLc("let", *$3, $5);
                delete $3;
            }
        | TOK_FOR '(' arguments_call ')' list_comprehension_elements_or_expr
            {
                $$ = $5;

                /* transform for(i=...,j=...) -> for(i=...) for(j=...) */
                for (int i = $3->size()-1; i >= 0; i--) {
                  AssignmentList arglist;
                  arglist.push_back((*$3)[i]);
                  Expression *e = new ExpressionLc("for", arglist, $$);
                    $$ = e;
                }
                delete $3;
            }
        | TOK_IF '(' expr ')' list_comprehension_elements_or_expr
            {
              $$ = new ExpressionLc("if", $3, $5);
            }
        ;

list_comprehension_elements_or_expr:
          list_comprehension_elements
        | expr
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

arguments_decl:
          /* empty */
            {
                $$ = new AssignmentList();
            }
        | argument_decl
            {
                $$ = new AssignmentList();
                $$->push_back(*$1);
                delete $1;
            }
        | arguments_decl ',' optional_commas argument_decl
            {
                $$ = $1;
                $$->push_back(*$4);
                delete $4;
            }
        ;

argument_decl:
          TOK_ID
            {
                $$ = new Assignment($1);
                free($1);
            }
        | TOK_ID '=' expr
            {
                $$ = new Assignment($1, boost::shared_ptr<Expression>($3));
                free($1);
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
                $$ = new Assignment("", boost::shared_ptr<Expression>($1));
            }
        | TOK_ID '=' expr
            {
                $$ = new Assignment($1, boost::shared_ptr<Expression>($3));
                free($1);
            }
        ;

%%

int parserlex(void)
{
  return lexerlex();
}

void yyerror (char const *s)
{
  // FIXME: We leak memory on parser errors...
  PRINTB("ERROR: Parser error in line %d: %s\n", lexerget_lineno() % s);
}

FileModule *parse(const char *text, const char *path, int debug)
{
  lexerin = NULL;
  parser_error_pos = -1;
  parser_input_buffer = text;
  parser_source_path = boosty::absolute(std::string(path)).string();

  rootmodule = new FileModule();
  rootmodule->setModulePath(path);
  scope_stack.push(&rootmodule->scope);
  //        PRINTB_NOCACHE("New module: %s %p", "root" % rootmodule);

  parserdebug = debug;
  int parserretval = parserparse();
  lexerdestroy();
  lexerlex_destroy();

  if (parserretval != 0) return NULL;

  parser_error_pos = -1;
  scope_stack.pop();
  return rootmodule;
}

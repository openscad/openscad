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

%expect 2 /* Expect 2 shift/reduce conflict for ifelse_statement - "dangling else problem" */

%{

#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "FileModule.h"
#include "UserModule.h"
#include "ModuleInstantiation.h"
#include "Assignment.h"
#include "expression.h"
#include "value.h"
#include "function.h"
#include "printutils.h"
#include "memory.h"
#include <sstream>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#define YYMAXDEPTH 20000
#define LOC(loc) Location(loc.first_line, loc.first_column, loc.last_line, loc.last_column)
  
int parser_error_pos = -1;

int parserlex(void);
void yyerror(char const *s);

int lexerget_lineno(void);
fs::path sourcefile(void);
int lexerlex_destroy(void);
int lexerlex(void);

std::stack<LocalScope *> scope_stack;
FileModule *rootmodule;

extern void lexerdestroy();
extern FILE *lexerin;
extern const char *parser_input_buffer;
const char *parser_input_buffer;
fs::path parser_sourcefile;

%}

%union {
  char *text;
  double number;
  class Value *value;
  class Expression *expr;
  class Vector *vec;
  class ModuleInstantiation *inst;
  class IfElseModuleInstantiation *ifelse;
  class Assignment *arg;
  AssignmentList *args;
}

%token TOK_ERROR

%token TOK_MODULE
%token TOK_FUNCTION
%token TOK_IF
%token TOK_ELSE
%token TOK_FOR
%token TOK_LET
%token TOK_ASSERT
%token TOK_ECHO
%token TOK_EACH

%token <text> TOK_ID
%token <text> TOK_STRING
%token <text> TOK_USE
%token <number> TOK_NUMBER

%token TOK_TRUE
%token TOK_FALSE
%token TOK_UNDEF

%token LE GE EQ NE AND OR

%right LET
%right LOW_PRIO_RIGHT
%left LOW_PRIO_LEFT

%right '?' ':'

%left OR
%left AND

%left '<' LE GE '>'
%left EQ NE

%left '!' '+' '-'
%left '*' '/' '%'
%left '[' ']'
%left '.'

%right HIGH_PRIO_RIGHT
%left HIGH_PRIO_LEFT

%type <expr> expr
%type <vec> vector_expr
%type <expr> list_comprehension_elements
%type <expr> list_comprehension_elements_p
%type <expr> list_comprehension_elements_or_expr
%type <expr> expr_or_empty

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
            {
              rootmodule->registerUse(std::string($1));
              free($1);
            }
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
              UserModule *newmodule = new UserModule($2, LOC(@$));
              newmodule->definition_arguments = *$4;
              scope_stack.top()->addModule($2, newmodule);
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
              UserFunction *func = UserFunction::create($2, *$4, shared_ptr<Expression>($8), LOC(@$));
              scope_stack.top()->addFunction(func);
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
                for (auto &assignment : scope_stack.top()->assignments) {
                    if (assignment.name == $1) {
                        assignment.expr = shared_ptr<Expression>($3);
                        assignment.setLocation(LOC(@$));
                        found = true;
                        break;
                    }
                }
                if (!found) {
                  scope_stack.top()->addAssignment(Assignment($1, shared_ptr<Expression>($3), LOC(@$)));
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
                $<ifelse>$ = new IfElseModuleInstantiation(shared_ptr<Expression>($3), parser_sourcefile.parent_path().generic_string(), LOC(@$));
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

// "for", "let" and "each" are valid module identifiers
module_id:
          TOK_ID  { $$ = $1; }
        | TOK_FOR { $$ = strdup("for"); }
        | TOK_LET { $$ = strdup("let"); }
        | TOK_ASSERT { $$ = strdup("assert"); }
        | TOK_ECHO { $$ = strdup("echo"); }
        | TOK_EACH { $$ = strdup("each"); }
        ;

single_module_instantiation:
          module_id '(' arguments_call ')'
            {
                $$ = new ModuleInstantiation($1, *$3, parser_sourcefile.parent_path().generic_string(), LOC(@$));
                free($1);
                delete $3;
            }
        ;

expr:
          TOK_TRUE
            {
              $$ = new Literal(ValuePtr(true), LOC(@$));
            }
        | TOK_FALSE
            {
              $$ = new Literal(ValuePtr(false), LOC(@$));
            }
        | TOK_UNDEF
            {
              $$ = new Literal(ValuePtr::undefined, LOC(@$));
            }
        | TOK_ID
            {
              $$ = new Lookup($1, LOC(@$));
                free($1);
            }
        | expr '.' TOK_ID
            {
              $$ = new MemberLookup($1, $3, LOC(@$));
              free($3);
            }
        | TOK_STRING
            {
              $$ = new Literal(ValuePtr(std::string($1)), LOC(@$));
              free($1);
            }
        | TOK_NUMBER
            {
              $$ = new Literal(ValuePtr($1), LOC(@$));
            }
        | '[' expr ':' expr ']'
            {
              $$ = new Range($2, $4, LOC(@$));
            }
        | '[' expr ':' expr ':' expr ']'
            {
              $$ = new Range($2, $4, $6, LOC(@$));
            }
        | '[' optional_commas ']'
            {
              $$ = new Literal(ValuePtr(Value::VectorType()), LOC(@$));
            }
        | '[' vector_expr optional_commas ']'
            {
              $$ = $2;
            }
        | expr '*' expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Multiply, $3, LOC(@$));
            }
        | expr '/' expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Divide, $3, LOC(@$));
            }
        | expr '%' expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Modulo, $3, LOC(@$));
            }
        | expr '+' expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Plus, $3, LOC(@$));
            }
        | expr '-' expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Minus, $3, LOC(@$));
            }
        | expr '<' expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Less, $3, LOC(@$));
            }
        | expr LE expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::LessEqual, $3, LOC(@$));
            }
        | expr EQ expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Equal, $3, LOC(@$));
            }
        | expr NE expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::NotEqual, $3, LOC(@$));
            }
        | expr GE expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::GreaterEqual, $3, LOC(@$));
            }
        | expr '>' expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Greater, $3, LOC(@$));
            }
        | expr AND expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::LogicalAnd, $3, LOC(@$));
            }
        | expr OR expr
            {
              $$ = new BinaryOp($1, BinaryOp::Op::LogicalOr, $3, LOC(@$));
            }
        | '+' expr
            {
                $$ = $2;
            }
        | '-' expr
            {
              $$ = new UnaryOp(UnaryOp::Op::Negate, $2, LOC(@$));
            }
        | '!' expr
            {
              $$ = new UnaryOp(UnaryOp::Op::Not, $2, LOC(@$));
            }
        | '(' expr ')'
            {
              $$ = $2;
            }
        | expr '?' expr ':' expr
            {
              $$ = new TernaryOp($1, $3, $5, LOC(@$));
            }
        | expr '[' expr ']'
            {
              $$ = new ArrayLookup($1, $3, LOC(@$));
            }
        | TOK_ID '(' arguments_call ')'
            {
              $$ = new FunctionCall($1, *$3, LOC(@$));
              free($1);
              delete $3;
            }
        | TOK_LET '(' arguments_call ')' expr %prec LET
            {
              $$ = FunctionCall::create("let", *$3, $5, LOC(@$));
              delete $3;
            }
        | TOK_ASSERT '(' arguments_call ')' expr_or_empty %prec LOW_PRIO_LEFT
            {
              $$ = FunctionCall::create("assert", *$3, $5, LOC(@$));
              delete $3;
            }
        | TOK_ECHO '(' arguments_call ')' expr_or_empty %prec LOW_PRIO_LEFT
            {
              $$ = FunctionCall::create("echo", *$3, $5, LOC(@$));
              delete $3;
            }
        ;

expr_or_empty:
          %prec LOW_PRIO_LEFT
            {
              $$ = NULL;
            }
        | expr %prec HIGH_PRIO_LEFT
            {
              $$ = $1;
            }
        ;
 
 list_comprehension_elements:
          /* The last set element may not be a "let" (as that would instead
             be parsed as an expression) */
          TOK_LET '(' arguments_call ')' list_comprehension_elements_p
            {
              $$ = new LcLet(*$3, $5, LOC(@$));
              delete $3;
            }
        | TOK_EACH list_comprehension_elements_or_expr
            {
              $$ = new LcEach($2, LOC(@$));
            }
        | TOK_FOR '(' arguments_call ')' list_comprehension_elements_or_expr
            {
                $$ = $5;

                /* transform for(i=...,j=...) -> for(i=...) for(j=...) */
                for (int i = $3->size()-1; i >= 0; i--) {
                  AssignmentList arglist;
                  arglist.push_back((*$3)[i]);
                  Expression *e = new LcFor(arglist, $$, LOC(@$));
                    $$ = e;
                }
                delete $3;
            }
        | TOK_FOR '(' arguments_call ';' expr ';' arguments_call ')' list_comprehension_elements_or_expr
            {
              $$ = new LcForC(*$3, *$7, $5, $9, LOC(@$));
                delete $3;
                delete $7;
            }
        | TOK_IF '(' expr ')' list_comprehension_elements_or_expr
            {
              $$ = new LcIf($3, $5, 0, LOC(@$));
            }
        | TOK_IF '(' expr ')' list_comprehension_elements_or_expr TOK_ELSE list_comprehension_elements_or_expr
            {
              $$ = new LcIf($3, $5, $7, LOC(@$));
            }
        ;

// list_comprehension_elements with optional parenthesis
list_comprehension_elements_p:
          list_comprehension_elements
        | '(' list_comprehension_elements ')'
            {
                $$ = $2;
            }
        ;

list_comprehension_elements_or_expr:
          list_comprehension_elements_p
        | expr
        ;

optional_commas:
          ',' optional_commas
        | /* empty */
        ;

vector_expr:
          expr
            {
              $$ = new Vector(LOC(@$));
              $$->push_back($1);
            }
        |  list_comprehension_elements
            {
              $$ = new Vector(LOC(@$));
              $$->push_back($1);
            }
        | vector_expr ',' optional_commas list_comprehension_elements_or_expr
            {
              $$ = $1;
              $$->push_back($4);
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
                $$ = new Assignment($1, LOC(@$));
                free($1);
            }
        | TOK_ID '=' expr
            {
              $$ = new Assignment($1, shared_ptr<Expression>($3), LOC(@$));
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
                $$ = new Assignment("", shared_ptr<Expression>($1), LOC(@$));
            }
        | TOK_ID '=' expr
            {
                $$ = new Assignment($1, shared_ptr<Expression>($3), LOC(@$));
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
  PRINTB("ERROR: Parser error in file %s, line %d: %s\n",
         sourcefile() % lexerget_lineno() % s);
}

bool parse(FileModule *&module, const char *text, const std::string &filename, int debug)
{
  fs::path path = fs::absolute(fs::path(filename));
  
  lexerin = NULL;
  parser_error_pos = -1;
  parser_input_buffer = text;
  parser_sourcefile = path;

  rootmodule = new FileModule(path.parent_path().generic_string(), path.filename().generic_string());
  scope_stack.push(&rootmodule->scope);
  //        PRINTB_NOCACHE("New module: %s %p", "root" % rootmodule);

  parserdebug = debug;
  int parserretval = parserparse();

  lexerdestroy();
  lexerlex_destroy();

  module = rootmodule;
  if (parserretval != 0) return false;

  parser_error_pos = -1;
  scope_stack.pop();

  return true;
}

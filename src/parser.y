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

%token TOK_MODULE
%token TOK_FUNCTION
%token TOK_IF
%token TOK_ELSE

%token <text> TOK_ID
%token <text> TOK_STRING
%token <text> TOK_USE
%token <number> TOK_NUMBER

%token TOK_TRUE
%token TOK_FALSE
%token TOK_UNDEF

%token LE GE EQ NE AND OR

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

%type <inst> module_instantiation
%type <ifelse> if_statement
%type <ifelse> ifelse_statement
%type <inst> single_module_instantiation

%type <args> arguments_call
%type <args> arguments_decl

%type <arg> argument_call
%type <arg> argument_decl

%debug

%%

input: 
/* empty */ |
TOK_USE { rootmodule->usedlibs.insert($1); } input |
statement input ;

inner_input: 
/* empty */ |
statement inner_input ;

assignment:
TOK_ID '=' expr ';' {
  bool found = false;
  for (AssignmentList::iterator iter = scope_stack.top()->assignments.begin(); 
       iter != scope_stack.top()->assignments.end(); 
       iter++) {
    if (iter->first == $1) {
      iter->second = $3;
      found = true;
      break;
    }
  }
  if (!found) scope_stack.top()->assignments.push_back(Assignment($1, $3));
} ;

statement:
';' |
'{' inner_input '}' |
module_instantiation {
  if ($1) scope_stack.top()->addChild($1);
} |
assignment |
TOK_MODULE TOK_ID '(' arguments_decl optional_commas ')' {
  Module *newmodule = new Module();
  newmodule->definition_arguments = *$4;
  scope_stack.top()->modules[$2] = newmodule;
  scope_stack.push(&newmodule->scope);
  free($2);
  delete $4;
} statement {
  scope_stack.pop();
  } |
  TOK_FUNCTION TOK_ID '(' arguments_decl optional_commas ')' '=' expr {
    Function *func = new Function();
    func->definition_arguments = *$4;
    func->expr = $8;
    scope_stack.top()->functions[$2] = func;
    free($2);
    delete $4;
  } ';' ;

if_statement:
TOK_IF '(' expr ')' {
  $<ifelse>$ = new IfElseModuleInstantiation();
  $<ifelse>$->arguments.push_back(Assignment("", $3));
  $<ifelse>$->setPath(parser_source_path);
  scope_stack.push(&$<ifelse>$->scope);
} child_statement {
  scope_stack.pop();
  $$ = $<ifelse>5;
 } ;

ifelse_statement:
if_statement {
  $$ = $1;
} |
if_statement TOK_ELSE {
  scope_stack.push(&$1->else_scope);
} child_statement {
  scope_stack.pop();
  $$ = $1;
 } ;

module_instantiation:
'!' module_instantiation {
  $$ = $2;
  if ($$) $$->tag_root = true;
} |
'#' module_instantiation {
  $$ = $2;
  if ($$) $$->tag_highlight = true;
} |
'%' module_instantiation {
  $$ = $2;
  if ($$) $$->tag_background = true;
} |
'*' module_instantiation {
  delete $2;
  $$ = NULL;
} |
single_module_instantiation {
  $<inst>$ = $1;
  scope_stack.push(&$1->scope);
} child_statement {
  scope_stack.pop();
  $$ = $<inst>2;
 } |
 ifelse_statement {
   $$ = $1;
 } ;

child_statement:
';' |
'{' child_statements '}' |
module_instantiation {
  if ($1) scope_stack.top()->addChild($1);
} ;

/*
 FIXME: This allows for variable declaration in child blocks, not activated yet
 |
assignment ;
*/

child_statements:
/* empty */ |
child_statements child_statement ;

single_module_instantiation:
TOK_ID '(' arguments_call ')' {
  $$ = new ModuleInstantiation($1);
  $$->arguments = *$3;
  $$->setPath(parser_source_path);
  free($1);
  delete $3;
}

expr:
TOK_TRUE {
  $$ = new Expression(Value(true));
} |
TOK_FALSE {
  $$ = new Expression(Value(false));
} |
TOK_UNDEF {
  $$ = new Expression(Value::undefined);
} |
TOK_ID {
  $$ = new Expression();
  $$->type = "L";
  $$->var_name = $1;
  free($1);
} |
expr '.' TOK_ID {
  $$ = new Expression("N", $1);
  $$->var_name = $3;
  free($3);
} |
TOK_STRING {
  $$ = new Expression(Value(std::string($1)));
  free($1);
} |
TOK_NUMBER {
  $$ = new Expression(Value($1));
} |
'[' expr ':' expr ']' {
  Expression *e_one = new Expression(Value(1.0));
  $$ = new Expression();
  $$->type = "R";
  $$->children.push_back($2);
  $$->children.push_back(e_one);
  $$->children.push_back($4);
} |
'[' expr ':' expr ':' expr ']' {
  $$ = new Expression();
  $$->type = "R";
  $$->children.push_back($2);
  $$->children.push_back($4);
  $$->children.push_back($6);
} |
'[' optional_commas ']' {
  $$ = new Expression(Value(Value::VectorType()));
} |
'[' vector_expr optional_commas ']' {
  $$ = $2;
} |
expr '*' expr {
  $$ = new Expression("*", $1, $3);
} |
expr '/' expr {
  $$ = new Expression("/", $1, $3);
} |
expr '%' expr {
  $$ = new Expression("%", $1, $3);
} |
expr '+' expr {
  $$ = new Expression("+", $1, $3);
} |
expr '-' expr {
  $$ = new Expression("-", $1, $3);
} |
expr '<' expr {
  $$ = new Expression("<", $1, $3);
} |
expr LE expr {
  $$ = new Expression("<=", $1, $3);
} |
expr EQ expr {
  $$ = new Expression("==", $1, $3);
} |
expr NE expr {
  $$ = new Expression("!=", $1, $3);
} |
expr GE expr {
  $$ = new Expression(">=", $1, $3);
} |
expr '>' expr {
  $$ = new Expression(">", $1, $3);
} |
expr AND expr {
  $$ = new Expression("&&", $1, $3);
} |
expr OR expr {
  $$ = new Expression("||", $1, $3);
} |
'+' expr {
  $$ = $2;
} |
'-' expr {
  $$ = new Expression("I", $2);
} |
'!' expr {
  $$ = new Expression("!", $2);
} |
'(' expr ')' {
  $$ = $2;
} |
expr '?' expr ':' expr {
  $$ = new Expression();
  $$->type = "?:";
  $$->children.push_back($1);
  $$->children.push_back($3);
  $$->children.push_back($5);
} |
expr '[' expr ']' {
  $$ = new Expression("[]", $1, $3);
} |
TOK_ID '(' arguments_call ')' {
  $$ = new Expression();
  $$->type = "F";
  $$->call_funcname = $1;
  $$->call_arguments = *$3;
  free($1);
  delete $3;
} ;

optional_commas:
',' optional_commas | ;

vector_expr:
expr {
  $$ = new Expression("V", $1);
} |
vector_expr ',' optional_commas expr {
  $$ = $1;
  $$->children.push_back($4);
} ;

arguments_decl:
/* empty */ {
  $$ = new AssignmentList();
} |
argument_decl {
  $$ = new AssignmentList();
  $$->push_back(*$1);
  delete $1;
} |
arguments_decl ',' optional_commas argument_decl {
  $$ = $1;
  $$->push_back(*$4);
  delete $4;
} ;

argument_decl:
TOK_ID {
  $$ = new Assignment($1, NULL);
  free($1);
} |
TOK_ID '=' expr {
  $$ = new Assignment($1, $3);
  free($1);
} ;

arguments_call:
/* empty */ {
  $$ = new AssignmentList();
} |
argument_call {
  $$ = new AssignmentList();
  $$->push_back(*$1);
  delete $1;
} |
arguments_call ',' optional_commas argument_call {
  $$ = $1;
  $$->push_back(*$4);
  delete $4;
} ;

argument_call:
expr {
  $$ = new Assignment("", $1);
} |
TOK_ID '=' expr {
  $$ = new Assignment($1, $3);
  free($1);
} ;

%%

int parserlex(void)
{
  return lexerlex();
}

void yyerror (char const *s)
{
  // FIXME: We leak memory on parser errors...
  PRINTB("Parser error in line %d: %s\n", lexerget_lineno() % s);
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

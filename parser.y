/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

%{

#include "openscad.h"

int parserlex(void);
void yyerror(char const *s);

int lexerget_lineno(void);
int lexerlex_destroy(void);
int lexerlex(void);

QVector<Module*> module_stack;
Module *module;

class ArgContainer {
public:
	QString argname;
	Expression *argexpr;
};
class ArgsContainer {
public:
	QVector<QString> argnames;
	QVector<Expression*> argexpr;
};

%}

%union {
	char *text;
	double number;
	class Expression *expr;
	class ModuleInstanciation *inst;
	class ArgContainer *arg;
	class ArgsContainer *args;
}

%token TOK_MODULE
%token TOK_FUNCTION

%token <text> TOK_ID
%token <text> TOK_STRING
%token <number> TOK_NUMBER

%token TOK_TRUE
%token TOK_FALSE
%token TOK_UNDEF

%left '+' '-'
%left '*' '/' '%'
%left '.'

%type <expr> expr

%type <inst> module_instantciation
%type <inst> module_instantciation_list
%type <inst> single_module_instantciation

%type <args> arguments_call
%type <args> arguments_decl

%type <arg> argument_call
%type <arg> argument_decl

%debug

%%

input: 
	/* empty */ |
	statement input ;

statement:
	';' |
	'{' input '}' |
	module_instantciation {
		module->children.append($1);
	} |
	TOK_ID '=' expr ';' {
		module->assignments_var.append($1);
		module->assignments_expr.append($3);
		free($1);
	} |
	TOK_MODULE TOK_ID '(' arguments_decl ')' {
		Module *p = module;
		module_stack.append(module);
		module = new Module();
		p->modules[$2] = module;
		module->argnames = $4->argnames;
		module->argexpr = $4->argexpr;
		free($2);
		delete $4;
	} statement {
		module = module_stack.last();
		module_stack.pop_back();
	} |
	TOK_FUNCTION TOK_ID '(' arguments_decl ')' '=' expr {
		Function *func = new Function();
		func->argnames = $4->argnames;
		func->argexpr = $4->argexpr;
		func->expr = $7;
		module->functions[$2] = func;
		free($2);
		delete $4;
	} ;

module_instantciation:
	single_module_instantciation ';' {
		$$ = $1;
	} |
	single_module_instantciation '{' module_instantciation_list '}' {
		$$ = $1;
		$$->children = $3->children;
		$3->children.clear();
		delete $3;
	} |
	single_module_instantciation module_instantciation {
		$$ = $1;
		$$->children.append($2);
	} ;

module_instantciation_list:
	/* empty */ {
		$$ = new ModuleInstanciation();
	} |
	module_instantciation_list module_instantciation {
		$$ = $1;
		$$->children.append($2);
	} ;

single_module_instantciation:
	TOK_ID '(' arguments_call ')' {
		$$ = new ModuleInstanciation();
		$$->modname = QString($1);
		$$->argnames = $3->argnames;
		$$->argexpr = $3->argexpr;
		free($1);
		delete $3;
	} |
	TOK_ID ':' TOK_ID '(' arguments_call ')' {
		$$ = new ModuleInstanciation();
		$$->label = QString($1);
		$$->modname = QString($3);
		$$->argnames = $5->argnames;
		$$->argexpr = $5->argexpr;
		free($1);
		free($3);
		delete $5;
	} ;
	
expr:
	TOK_TRUE {
		$$ = new Expression();
		$$->type = 'C';
		$$->const_value = Value(true);
	} |
	TOK_FALSE {
		$$ = new Expression();
		$$->type = 'C';
		$$->const_value = Value(false);
	} |
	TOK_UNDEF {
		$$ = new Expression();
		$$->type = 'C';
		$$->const_value = Value();
	} |
	TOK_ID {
		$$ = new Expression();
		$$->type = 'L';
		$$->var_name = QString($1);
		free($1);
	} |
	expr '.' TOK_ID {
		$$ = new Expression();
		$$->type = 'N';
		$$->children.append($1);
		$$->var_name = QString($3);
		free($3);
	} |
	TOK_STRING {
		$$ = new Expression();
		$$->type = 'C';
		$$->const_value = Value(QString($1));
		free($1);
	} |
	TOK_NUMBER {
		$$ = new Expression();
		$$->type = 'C';
		$$->const_value = Value($1);
	} |
	'[' expr ':' expr ']' {
		Expression *e_one = new Expression();
		e_one->type = 'C';
		e_one->const_value = Value(1.0);
		$$ = new Expression();
		$$->type = 'R';
		$$->children.append($2);
		$$->children.append(e_one);
		$$->children.append($4);
	} |
	'[' expr ':' expr ':' expr ']' {
		$$ = new Expression();
		$$->type = 'R';
		$$->children.append($2);
		$$->children.append($4);
		$$->children.append($6);
	} |
	'[' TOK_NUMBER TOK_NUMBER TOK_NUMBER ']' {
		$$ = new Expression();
		$$->type = 'C';
		$$->const_value = Value($2, $3, $4);
	} |
	'[' TOK_NUMBER TOK_NUMBER TOK_NUMBER TOK_NUMBER ';'
	    TOK_NUMBER TOK_NUMBER TOK_NUMBER TOK_NUMBER ';'
	    TOK_NUMBER TOK_NUMBER TOK_NUMBER TOK_NUMBER ';'
	    TOK_NUMBER TOK_NUMBER TOK_NUMBER TOK_NUMBER ']' {
		$$ = new Expression();
		$$->type = 'C';
		double m[16] = {
			 $2,  $3,  $4,  $5,
			 $7,  $8,  $9, $10,
			$12, $13, $14, $15,
			$17, $18, $19, $20,
		};
		$$->const_value = Value(m);
	} |
	'[' expr ',' expr ',' expr ']' {
		$$ = new Expression();
		$$->type = 'V';
		$$->children.append($2);
		$$->children.append($4);
		$$->children.append($6);
	} |
	'[' expr ',' expr ',' expr ',' expr ';'
	    expr ',' expr ',' expr ',' expr ';'
	    expr ',' expr ',' expr ',' expr ';'
	    expr ',' expr ',' expr ',' expr ']' {
		$$ = new Expression();
		$$->type = 'M';
		$$->children.append($2);
		$$->children.append($4);
		$$->children.append($6);
		$$->children.append($8);
		$$->children.append($10);
		$$->children.append($12);
		$$->children.append($14);
		$$->children.append($16);
		$$->children.append($18);
		$$->children.append($20);
		$$->children.append($22);
		$$->children.append($24);
		$$->children.append($26);
		$$->children.append($28);
		$$->children.append($30);
		$$->children.append($32);
	} |
	expr '*' expr {
		$$ = new Expression();
		$$->type = '*';
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '/' expr {
		$$ = new Expression();
		$$->type = '/';
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '%' expr {
		$$ = new Expression();
		$$->type = '%';
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '+' expr {
		$$ = new Expression();
		$$->type = '+';
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '-' expr {
		$$ = new Expression();
		$$->type = '-';
		$$->children.append($1);
		$$->children.append($3);
	} |
	'+' expr {
		$$ = $2;
	} |
	'-' expr {
		$$ = new Expression();
		$$->type = 'I';
		$$->children.append($2);
	} |
	'(' expr ')' {
		$$ = $2;
	} |
	expr '?' expr ':' expr {
		$$ = new Expression();
		$$->type = '?';
		$$->children.append($1);
		$$->children.append($3);
		$$->children.append($5);
	} |
	TOK_ID '(' arguments_call ')' {
		$$ = new Expression();
		$$->type = 'F';
		$$->call_funcname = QString($1);
		$$->call_argnames = $3->argnames;
		$$->children = $3->argexpr;
		free($1);
		delete $3;
	} ;

arguments_decl:
	/* empty */ {
		$$ = new ArgsContainer();
	} |
	argument_decl {
		$$ = new ArgsContainer();
		$$->argnames.append($1->argname);
		$$->argexpr.append($1->argexpr);
		delete $1;
	} |
	arguments_decl ',' argument_decl {
		$$ = $1;
		$$->argnames.append($3->argname);
		$$->argexpr.append($3->argexpr);
		delete $3;
	} ;

argument_decl:
	TOK_ID {
		$$ = new ArgContainer();
		$$->argname = QString($1);
		$$->argexpr = NULL;
		free($1);
	} |
	TOK_ID '=' expr {
		$$ = new ArgContainer();
		$$->argname = QString($1);
		$$->argexpr = $3;
		free($1);
	} ;

arguments_call:
	/* empty */ {
		$$ = new ArgsContainer();
	} |
	argument_call {
		$$ = new ArgsContainer();
		$$->argnames.append($1->argname);
		$$->argexpr.append($1->argexpr);
		delete $1;
	} |
	arguments_call ',' argument_call {
		$$ = $1;
		$$->argnames.append($3->argname);
		$$->argexpr.append($3->argexpr);
		delete $3;
	} ;

argument_call:
	expr {
		$$ = new ArgContainer();
		$$->argexpr = $1;
	} |
	TOK_ID '=' expr {
		$$ = new ArgContainer();
		$$->argname = QString($1);
		$$->argexpr = $3;
		free($1);
	} ;

%%

int parserlex(void)
{
	return lexerlex();
}

void yyerror (char const *s)
{
	fprintf(stderr, "Parser error in line %d: %s\n", lexerget_lineno(), s);
	exit(1);
}

extern const char *parser_input_buffer;
const char *parser_input_buffer;

AbstractModule *parse(const char *text, int debug)
{
	parser_input_buffer = text;

	module_stack.clear();
	module = new Module();

	parserdebug = debug;
	parserparse();

	lexerlex_destroy();

	return module;
}


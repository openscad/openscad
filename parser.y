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
int lexerlex(void);

QVector<Module*> module_stack;
Module *module;

%}

%union {
	char *text;
	double number;
}

%token TOK_MODULE
%token TOK_FUNCTION
%token <text> TOK_ID
%token <text> TOK_STRING
%token <number> TOK_NUMBER

%left '+' '-'
%left '*' '/' '%'
%left '.'

%debug

%%

input: 
	/* empty */ |
	statement input ;

statement:
	';' |
	'{' input '}' |
	node statement |
	TOK_ID '=' expr ';' |
	TOK_MODULE TOK_ID '(' arguments_decal ')' statement |
	TOK_FUNCTION TOK_ID '(' arguments_decal ')' expr ;

node:
	TOK_ID '(' arguments_call ')' |
	TOK_ID ':' TOK_ID '(' arguments_call ')' ;

expr:
	TOK_ID |
	expr '.' TOK_ID |
	TOK_STRING |
	TOK_NUMBER |
	TOK_NUMBER ':' TOK_NUMBER |
	TOK_NUMBER ':' TOK_NUMBER ':' TOK_NUMBER |
	'[' TOK_NUMBER TOK_NUMBER TOK_NUMBER ']' |
	'[' expr ',' expr ',' expr ']' |
	expr '*' expr |
	expr '/' expr |
	expr '%' expr |
	expr '+' expr |
	expr '-' expr |
	'+' expr |
	'-' expr |
	'(' expr ')' |
	TOK_ID '(' arguments_call ')' ;

arguments_decal:
	argument_decal |
	argument_decal ',' arguments_decal |
	/* empty */ ;

argument_decal:
	TOK_ID |
	TOK_ID '=' expr ;

arguments_call:
	argument_call |
	argument_call ',' arguments_call |
	/* empty */ ;

argument_call:
	expr |
	TOK_ID '=' expr ;

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

AbstractModule *parse(FILE *f, int debug)
{
	module_stack.clear();
	module = new Module();
	module_stack.append(module);

	parserdebug = debug;
	parserparse();

	return module;
}


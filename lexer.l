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
#include "parser_yacc.h"

%}

%option yylineno
%option noyywrap

%x comment

WS [\n\r\t ]

%%

[0-9][0-9.]* { parserlval.number = atof(yytext); return TOK_NUMBER; }
[a-zA-Z0-9_]+ { parserlval.text = strdup(yytext); return TOK_ID; }
\"[^"]*\" { parserlval.text = strdup(yytext); return TOK_STRING; }

"."	return '.';
","	return ',';
";"	return ';';
":"	return ':';
"="	return '=';
"*"	return '*';
"/"	return '/';
"%"	return '%';
"+"	return '+';
"-"	return '-';
"("	return '(';
")"	return ')';
"{"	return '{';
"}"	return '}';
"["	return '[';
"]"	return ']';

"module"/{WS}	return TOK_MODULE;
"function"/{WS}	return TOK_FUNCTION;

\/\/[^\n]*\n
"/*" BEGIN(comment);
<comment>"*/" BEGIN(INITIAL);
<comment>.|\n

{WS}*

. { fprintf(stderr, "Unrecognized input character in line %d: %s\n", lexerget_lineno(), yytext); exit(1); }


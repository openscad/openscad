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

int lexerget_lineno(void);
extern const char *parser_input_buffer;

#define YY_INPUT(buf,result,max_size) { \
  if (*parser_input_buffer) {           \
    result = 1;                         \
    buf[0] = *(parser_input_buffer++);  \
  } else {                              \
    result = YY_NULL;                   \
  }                                     \
}

%}

%option yylineno
%option noyywrap

%x comment

%%

"module"	return TOK_MODULE;
"function"	return TOK_FUNCTION;

"true"		return TOK_TRUE;
"false"		return TOK_FALSE;
"undef"		return TOK_UNDEF;

[+-]?[0-9][0-9.]* { parserlval.number = atof(yytext); return TOK_NUMBER; }
"$"?[a-zA-Z0-9_]+ { parserlval.text = strdup(yytext); return TOK_ID; }
\"[^"]*\" { parserlval.text = strdup(yytext); return TOK_STRING; }

"."	return '.';
","	return ',';
";"	return ';';
"?"	return '?';
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

[\n\r\t ]
\/\/[^\n]*\n
"/*" BEGIN(comment);
<comment>"*/" BEGIN(INITIAL);
<comment>.|\n

. { fprintf(stderr, "Unrecognized input character in line %d: %s\n", lexerget_lineno(), yytext); exit(1); }


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
#include "printutils.h"
#include "parser_yacc.h"

int lexerget_lineno(void);
#ifdef __GNUC__
static void yyunput(int, char*) __attribute__((unused));
#endif
extern const char *parser_input_buffer;

#define YY_INPUT(buf,result,max_size) {   \
  if (yyin && yyin != stdin) {            \
    int c = fgetc(yyin);                  \
    if (c >= 0) {                         \
      result = 1;                         \
      buf[0] = c;                         \
    } else {                              \
      result = YY_NULL;                   \
    }                                     \
  } else {                                \
    if (*parser_input_buffer) {           \
      result = 1;                         \
      buf[0] = *(parser_input_buffer++);  \
      parser_error_pos++;                 \
    } else {                              \
      result = YY_NULL;                   \
    }                                     \
  }                                       \
}

%}

%option yylineno
%option noyywrap

%x comment

%%

"<"[^ \t\n>]+">" {
	char *filename = strdup(yytext+1);
	filename[strlen(filename)-1] = 0;
	handle_dep(filename);
	yyin = fopen(filename, "r");
	if (!yyin) {
		PRINTF("WARNING: Can't open input file `%s'.", filename);
	} else {
		yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE ));
		BEGIN(INITIAL);
	}
	free(filename);
}

<<EOF>> {
	if (yyin && yyin != stdin)
		fclose(yyin);
	yypop_buffer_state();
	if (!YY_CURRENT_BUFFER)
		yyterminate();
}

"module"	return TOK_MODULE;
"function"	return TOK_FUNCTION;

"true"		return TOK_TRUE;
"false"		return TOK_FALSE;
"undef"		return TOK_UNDEF;

[0-9][0-9.]* { parserlval.number = atof(yytext); return TOK_NUMBER; }
"$"?[a-zA-Z0-9_]+ { parserlval.text = strdup(yytext); return TOK_ID; }

\"[^"]*\" {
	parserlval.text = strdup(yytext+1);
	parserlval.text[strlen(parserlval.text)-1] = 0;
	return TOK_STRING;
}

[\n\r\t ]
\/\/[^\n]*\n
"/*" BEGIN(comment);
<comment>"*/" BEGIN(INITIAL);
<comment>.|\n

"<="	return LE;
">="	return GE;
"=="	return EQ;
"!="	return NE;
"&&"	return AND;
"||"	return OR;

. { return yytext[0]; }


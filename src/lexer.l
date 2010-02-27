/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
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

%{

#include "openscad.h"
#include "printutils.h"
#include "parser_yacc.h"
#include <QFileInfo>
#include <QDir>

int lexerget_lineno(void);
#ifdef __GNUC__
static void yyunput(int, char*) __attribute__((unused));
#endif
extern const char *parser_input_buffer;
extern const char *parser_source_path;

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

include[ \t\r\n>]*"<"[^ \t\r\n>]+">" {
	QString filename(yytext);
	filename.remove(QRegExp("^include[ \t\r\n>]*<"));
	filename.remove(QRegExp(">$"));
	QFileInfo finfo(QDir(parser_source_path), filename);
	if (!finfo.exists()) {
		finfo = QFileInfo(QDir(librarydir), filename);
	}
	handle_dep(finfo.absoluteFilePath());
	yyin = fopen(finfo.absoluteFilePath().toLocal8Bit(), "r");
	if (!yyin) {
		PRINTA("WARNING: Can't open input file `%1'.", filename);
	} else {
		yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE ));
		BEGIN(INITIAL);
	}
}

use[ \t\r\n>]*"<"[^ \t\r\n>]+">" {
	QString filename(yytext);
	filename.remove(QRegExp("^use[ \t\r\n>]*<"));
	filename.remove(QRegExp(">$"));
	parserlval.text = strdup(filename.toLocal8Bit());
	return TOK_USE;
}

"<"[^ \t\r\n>]+">" {
	char *filename = strdup(yytext+1);
	filename[strlen(filename)-1] = 0;
	QFileInfo finfo(QDir(parser_source_path), filename);
	if (!finfo.exists()) {
		finfo = QFileInfo(QDir(librarydir), filename);
	}

	PRINTF("WARNING: The use of implicit include is depricated. Use `include <filename>' instead.");
	handle_dep(finfo.absoluteFilePath());
	yyin = fopen(finfo.absoluteFilePath().toLocal8Bit(), "r");
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
"if"		return TOK_IF;
"else"		return TOK_ELSE;

"true"		return TOK_TRUE;
"false"		return TOK_FALSE;
"undef"		return TOK_UNDEF;

[0-9][0-9.]* { parserlval.number = QString(yytext).toDouble(); return TOK_NUMBER; }
"$"?[a-zA-Z0-9_]+ { parserlval.text = strdup(yytext); return TOK_ID; }

\"[^"]*\" {
	parserlval.text = strdup(yytext+1);
	parserlval.text[strlen(parserlval.text)-1] = 0;
	return TOK_STRING;
}

[\n\r\t ]
\/\/[^\n]*\n?
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


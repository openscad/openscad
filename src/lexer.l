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

%{

#include "handle_dep.h"
#include "openscad.h" // librarydir
#include "printutils.h"
#include "parser_yacc.h"
#include <QStack>
#include <QFileInfo>
#include <QDir>
#include <assert.h>

//isatty for visual c++ and mingw-cross-env
#if defined __WIN32__ && ! defined _MSC_VER
#include "unistd.h"
#endif
#if defined __WIN32__ || defined _MSC_VER
extern "C" int __cdecl _isatty(int _FileHandle);
#define isatty _isatty
#endif

QString* stringcontents;
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

void includefile();
QDir sourcepath();
QStack<QDir> path_stack;
QStack<FILE*> openfiles;

QString filename;
QString filepath;

%}

%option yylineno
%option noyywrap

%x comment string
%x include

DIGIT [0-9]

%%

include[ \t\r\n>]*"<"	{ BEGIN(include); }
<include>{
[^\t\r\n>]*"/"	{ filepath = yytext; }
[^\t\r\n>/]+	{ filename = yytext; }
">"		{ BEGIN(INITIAL); includefile(); }
}


use[ \t\r\n>]*"<"[^\t\r\n>]+">" {
	QString filename(yytext);
	filename.remove(QRegExp("^use[ \t\r\n>]*<"));
	filename.remove(QRegExp(">$"));
	QFileInfo finfo(QDir(parser_source_path), filename);
	if (!finfo.exists()) {
		finfo = QFileInfo(QDir(librarydir), filename);
	}
	handle_dep(finfo.absoluteFilePath().toStdString());
	parserlval.text = strdup(finfo.absoluteFilePath().toLocal8Bit());
	return TOK_USE;
}

"<"[^ \t\r\n>]+">" {
	char *filename = strdup(yytext+1);
	filename[strlen(filename)-1] = 0;
	QFileInfo finfo(QDir(parser_source_path), filename);
	if (!finfo.exists()) {
		finfo = QFileInfo(QDir(librarydir), filename);
	}

	PRINTF("DEPRECATED: Support for implicit include will be removed in future releases. Use `include <filename>' instead.");
	handle_dep(finfo.absoluteFilePath().toStdString());
	yyin = fopen(finfo.absoluteFilePath().toLocal8Bit(), "r");
	if (!yyin) {
		PRINTF("WARNING: Can't open input file `%s'.", filename);
	} else {
		openfiles.append(yyin);
		yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE ));
		BEGIN(INITIAL);
	}
	free(filename);
}

<<EOF>> {
	if(!path_stack.empty())
		path_stack.pop();
	if (yyin && yyin != stdin) {
		assert(!openfiles.empty());
		fclose(openfiles.pop());
	}
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

{DIGIT}+|{DIGIT}*\.{DIGIT}+|{DIGIT}+\.{DIGIT}* { parserlval.number = QString(yytext).toDouble(); return TOK_NUMBER; }
"$"?[a-zA-Z0-9_]+ { parserlval.text = strdup(yytext); return TOK_ID; }

\"			{ BEGIN(string); stringcontents = new QString(); }
<string>{
\\n			{ stringcontents->append('\n'); }
\\t			{ stringcontents->append('\t'); }
\\r			{ stringcontents->append('\r'); }
\\\\			{ stringcontents->append('\\'); }
\\\"			{ stringcontents->append('"'); }
[^\\\n\"]+		{ stringcontents->append(lexertext); }
\"			{ BEGIN(INITIAL); 
			parserlval.text = strdup(stringcontents->toLocal8Bit()); 
			delete stringcontents; 
			return TOK_STRING; }
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

%%

QDir sourcepath()
{
	if(!path_stack.empty())
		return path_stack.top();

	return QDir(parser_source_path);
}

void includefile()
{
	if(filename.isEmpty())
		return;

	if(filepath.isEmpty()) {
		path_stack.push(sourcepath());
	} else {
		QFileInfo dirinfo(sourcepath(),filepath);
		path_stack.push(dirinfo.dir());
		filepath.clear();
	}

	QFileInfo finfo(sourcepath(), filename);
	if (!finfo.exists()) {
		finfo = QFileInfo(QDir(librarydir), filename);
	}

	handle_dep(finfo.absoluteFilePath().toStdString());
	yyin = fopen(finfo.absoluteFilePath().toLocal8Bit(), "r");
	if (!yyin) {
		PRINTA("WARNING: Can't open input file `%1'.", filename);
		path_stack.pop();
		return;
	}
	openfiles.append(yyin);
	filename.clear();

	yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE ));
}

/*!
  In case of an error, this will make sure we clean up our custom data structures 
  and close all files.
*/
void lexerdestroy()
{
  foreach (FILE *f, openfiles) fclose(f);
  openfiles.clear();
  path_stack.clear();
}

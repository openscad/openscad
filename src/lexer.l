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

#include "typedefs.h"
#include "handle_dep.h"
#include "printutils.h"
#include "parsersettings.h"
#include "parser_yacc.h"
#include "module.h"
#include <assert.h>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include "boosty.h"

//isatty for visual c++ and mingw-cross-env
#if defined __WIN32__ && ! defined _MSC_VER
#include "unistd.h"
#endif
#if defined __WIN32__ || defined _MSC_VER
extern "C" int __cdecl _isatty(int _FileHandle);
#define isatty _isatty
#endif

std::string stringcontents;
int lexerget_lineno(void);
#ifdef __GNUC__
static void yyunput(int, char*) __attribute__((unused));
#endif
extern const char *parser_input_buffer;
extern std::string parser_source_path;
extern FileModule *rootmodule;

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
fs::path sourcepath();
std::vector<fs::path> path_stack;
std::vector<FILE*> openfiles;
std::vector<std::string> openfilenames;

std::string filename;
std::string filepath;

%}

%option yylineno
%option noyywrap

%x cond_comment cond_string
%x cond_include
%x cond_use

D [0-9]
E [Ee][+-]?{D}+

%%

include[ \t\r\n>]*"<"	{ BEGIN(cond_include); filepath = filename = ""; }
<cond_include>{
[^\t\r\n>]*"/"	{ filepath = yytext; }
[^\t\r\n>/]+	{ filename = yytext; }
">"		{ BEGIN(INITIAL); includefile(); }
}


use[ \t\r\n>]*"<"	{ BEGIN(cond_use); }
<cond_use>{
[^\t\r\n>]+	{ filename = yytext; }
 ">"		{ 
	BEGIN(INITIAL); 
        fs::path fullpath = find_valid_path(sourcepath(), fs::path(filename), &openfilenames);
	if (fullpath.empty()) {
          PRINTB("WARNING: Can't open library '%s'.", filename);
          parserlval.text = strdup(filename.c_str());
	} else {
          handle_dep(fullpath.string());
          parserlval.text = strdup(fullpath.string().c_str());
	}
        return TOK_USE;
    }
}

<<EOF>> {
	if(!path_stack.empty()) path_stack.pop_back();
	if (yyin && yyin != stdin) {
		assert(!openfiles.empty());
		fclose(openfiles.back());
		openfiles.pop_back();
		openfilenames.pop_back();
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

{D}+{E}? | 
{D}*\.{D}+{E}? |
{D}+\.{D}*{E}?          { parserlval.number = boost::lexical_cast<double>(yytext); return TOK_NUMBER; }
"$"?[a-zA-Z0-9_]+       { parserlval.text = strdup(yytext); return TOK_ID; }

\"			{ BEGIN(cond_string); stringcontents.clear(); }
<cond_string>{
\\n			{ stringcontents += '\n'; }
\\t			{ stringcontents += '\t'; }
\\r			{ stringcontents += '\r'; }
\\\\			{ stringcontents += '\\'; }
\\\"			{ stringcontents += '"'; }
[^\\\n\"]+		{ stringcontents += lexertext; }
\"			{ BEGIN(INITIAL); 
			parserlval.text = strdup(stringcontents.c_str()); 
			return TOK_STRING; }
}

[\n\r\t ]
\/\/[^\n]*\n?
"/*" BEGIN(cond_comment);
<cond_comment>"*/" BEGIN(INITIAL);
<cond_comment>.|\n

"<="	return LE;
">="	return GE;
"=="	return EQ;
"!="	return NE;
"&&"	return AND;
"||"	return OR;

. { return yytext[0]; }

%%

fs::path sourcepath()
{
  if (!path_stack.empty()) return path_stack.back();
  
  return fs::path(parser_source_path);
}

/*
  Rules for include <path/file>
  1) include <sourcepath/path/file>
  2) include <librarydir/path/file>

  Globals used: filepath, sourcepath, filename
 */
void includefile()
{
  fs::path localpath = fs::path(filepath) / filename;
  fs::path fullpath = find_valid_path(sourcepath(), localpath, &openfilenames);
  if (!fullpath.empty()) {
    rootmodule->registerInclude(boosty::stringy(localpath), boosty::stringy(fullpath));
  }
  else {
    rootmodule->registerInclude(boosty::stringy(localpath), boosty::stringy(localpath));
    PRINTB("WARNING: Can't open include file '%s'.", boosty::stringy(localpath));
    if (path_stack.size() > 0) path_stack.pop_back();
    return;
  };

  std::string fullname = boosty::stringy(fullpath);

  filepath.clear();
  path_stack.push_back(fullpath.parent_path());

  handle_dep(fullname);

  yyin = fopen(fullname.c_str(), "r");
  if (!yyin) {
    PRINTB("WARNING: Can't open include file '%s'.", boosty::stringy(localpath));
    path_stack.pop_back();
    return;
  }

  openfiles.push_back(yyin);
  openfilenames.push_back(fullname);
  filename.clear();
  
  yypush_buffer_state(yy_create_buffer(yyin, YY_BUF_SIZE));
}

/*!
  In case of an error, this will make sure we clean up our custom data structures 
  and close all files.
*/
void lexerdestroy()
{
  BOOST_FOREACH (FILE *f, openfiles) fclose(f);
  openfiles.clear();
  openfilenames.clear();
  path_stack.clear();
}

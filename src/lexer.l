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

%option prefix="lexer"

%{

#include <glib.h>
#include "handle_dep.h"
#include "printutils.h"
#include "parsersettings.h"
#include "Assignment.h"
#include "parser.hxx"
#include "FileModule.h"
#include <assert.h>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include "boost-utils.h"
namespace fs = boost::filesystem;

//isatty for visual c++ and mingw-cross-env
#if defined __WIN32__ && ! defined _MSC_VER
#include "unistd.h"
#endif
#if defined __WIN32__ || defined _MSC_VER
extern "C" int __cdecl _isatty(int _FileHandle);
#define isatty _isatty
#define strdup _strdup
#define fileno _fileno
#endif

std::string stringcontents;
int lexerget_lineno(void);
#ifdef __GNUC__
static void yyunput(int, char*) __attribute__((unused));
#endif
extern const char *parser_input_buffer;
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

/*
  Handle locations.
  Since flex doesn't handle column numbers, we deal with those manually.
  See "Advanced Use of Flex" / "Advanced Use of Bison"
*/
extern YYLTYPE parserlloc;

#define LOCATION(loc) Location(loc.first_line, loc.first_column, loc.last_line, loc.last_column, sourcefile())
#define LOCATION_INIT(loc) do { (loc).first_line = (loc).first_column = (loc).last_line = (loc).last_column = yylineno = 1; } while (0)
#define LOCATION_NEXT(loc) do { (loc).first_column = (loc).last_column; (loc).first_line = (loc).last_line; } while (0)
#define LOCATION_ADD_LINES(loc, cnt) do { (loc).last_column = 1; (loc).last_line += cnt; LOCATION_NEXT(loc); } while (0)

#ifdef DEBUG
#define YY_USER_ACTION parserlloc.last_column += yyleng; \
        PRINTDB("YY_USER_ACTION(): %3d, %3d - %3d, %3d | %3d: %s", \
                parserlloc.first_line % parserlloc.first_column % \
				parserlloc.last_line % parserlloc.last_column % \
				yyleng % quoted_string(yytext));
#else
#define YY_USER_ACTION parserlloc.last_column += yyleng;
#endif

extern void parsererror(char const *s);
void to_utf8(const char *, char *);
void includefile(const Location& loc);
std::shared_ptr<fs::path> sourcefile();
std::shared_ptr<fs::path> parser_sourcefile;
std::vector<std::shared_ptr<fs::path>> filename_stack;
std::vector<YYLTYPE> loc_stack;
std::vector<FILE*> openfiles;
std::vector<std::string> openfilenames;

std::string filename;
std::string filepath;
%}

%option yylineno
%option noyywrap

%x cond_comment cond_lcomment cond_string
%x cond_include
%x cond_use

D [0-9]
E [Ee][+-]?{D}+
H [0-9a-fA-F]

U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]
UNICODE {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

%%

%{
LOCATION_NEXT(parserlloc);
%}

include[ \t\r\n]*"<"	{ BEGIN(cond_include); filepath = filename = ""; }
<cond_include>{
[^\t\r\n>]*"/"			{ filepath = yytext; }
[^\t\r\n>/]+			{ filename = yytext; }
">"						{ BEGIN(INITIAL); includefile(LOCATION(parserlloc));  }
<<EOF>>					{ parsererror("Unterminated include statement"); return TOK_ERROR; }
}


use[ \t\r\n]*"<"		{ BEGIN(cond_use); }
<cond_use>{
[^\t\r\n>]+				{ filename = yytext; }
 ">"					{
							BEGIN(INITIAL);
							fs::path fullpath = find_valid_path(sourcefile()->parent_path(), fs::path(filename), &openfilenames);
							if (fullpath.empty()) {
							LOG(message_group::Warning,Location::NONE,"","Can't open library '%1$s'.",filename);
								parserlval.text = strdup(filename.c_str());
							} else {
								handle_dep(fullpath.generic_string());
								parserlval.text = strdup(fullpath.string().c_str());
							}
							return TOK_USE;
						}
<<EOF>>					{ parsererror("Unterminated use statement"); return TOK_ERROR; }
}

\"						{ BEGIN(cond_string); stringcontents.clear(); }
<cond_string>{
\\n						{ stringcontents += '\n'; }
\\t						{ stringcontents += '\t'; }
\\r						{ stringcontents += '\r'; }
\\\\					{ stringcontents += '\\'; }
\\\"					{ stringcontents += '"'; }
{UNICODE}               { parser_error_pos -= strlen(lexertext) - 1; stringcontents += lexertext; }
\\x[0-7]{H}             { unsigned long i = strtoul(lexertext + 2, NULL, 16); stringcontents += (i == 0 ? ' ' : (unsigned char)(i & 0xff)); }
\\u{H}{4}|\\U{H}{6}     { char buf[8]; to_utf8(lexertext + 2, buf); stringcontents += buf; }
[^\\\n\"]				{ stringcontents += lexertext; }
[\n\r]					{ LOCATION_ADD_LINES(parserlloc, yyleng); }
\"						{ BEGIN(INITIAL); parserlval.text = strdup(stringcontents.c_str()); return TOK_STRING; }
<<EOF>>                 { parsererror("Unterminated string"); return TOK_ERROR; }
}

[\t ]                   { LOCATION_NEXT(parserlloc); }
[\n\r]					{ LOCATION_ADD_LINES(parserlloc, yyleng); }

\/\/					{ BEGIN(cond_lcomment); }
<cond_lcomment>{
\n                      { BEGIN(INITIAL); LOCATION_ADD_LINES(parserlloc, yyleng); }
{UNICODE}               { parser_error_pos -= strlen(lexertext) - 1; }
[^\n]
}

"/*" BEGIN(cond_comment);
<cond_comment>{
"*/"                    { BEGIN(INITIAL); }
{UNICODE}               { parser_error_pos -= strlen(lexertext) - 1; }
.
[\n]                    { LOCATION_ADD_LINES(parserlloc, yyleng); }
<<EOF>>                 { parsererror("Unterminated comment"); return TOK_ERROR; }
}

<<EOF>> {
	if (!filename_stack.empty()) filename_stack.pop_back();
	if (!loc_stack.empty()) {
		parserlloc = loc_stack.back();
		yylineno = parserlloc.first_line;
		loc_stack.pop_back();
	}
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

"\x03"		return TOK_EOT;

"module"	return TOK_MODULE;
"function"	return TOK_FUNCTION;
"if"		return TOK_IF;
"else"		return TOK_ELSE;
"let"		return TOK_LET;
"assert"	return TOK_ASSERT;
"echo"	        return TOK_ECHO;
"for"		return TOK_FOR;
"each"		return TOK_EACH;

"true"		return TOK_TRUE;
"false"		return TOK_FALSE;
"undef"		return TOK_UNDEF;

%{/*
 U+00A0 (UTF-8 encoded: C2A0) is no-break space. We support it since Qt's QTextEdit
 automatically converts these to spaces and we want to be able to process the same
 files on the cmd-line as in the editor.
*/%}

[\xc2\xa0]+

{UNICODE}+              { parser_error_pos -= strlen(yytext); return TOK_ERROR; }

{D}+{E}? |
{D}*\.{D}+{E}? |
{D}+\.{D}*{E}?          {
                            try {
                                parserlval.number = boost::lexical_cast<double>(yytext);
                                return TOK_NUMBER;
                            } catch (boost::bad_lexical_cast&) {}
                        }
"$"?[a-zA-Z0-9_]+       { parserlval.text = strdup(yytext); return TOK_ID; }

"<="	return LE;
">="	return GE;
"=="	return EQ;
"!="	return NE;
"&&"	return AND;
"||"	return OR;

. { return yytext[0]; }

%%

/*!
 * Convert unicode codepoint given in hex notation
 * into UTF8 encoding. The output buffer must be 8
 * characters long.
 */
void to_utf8(const char *str, char *out)
{
    memset(out, 0, 8);
    const gunichar c = strtoul(str, NULL, 16);
    if (g_unichar_validate(c) && (c != 0)) {
        g_unichar_to_utf8(c, out);
    } else {
        out[0] = ' ';
    }
}

void lexer_set_parser_sourcefile(const fs::path& path)
{
    parser_sourcefile = std::make_shared<fs::path>(path);
}

// Filename of the source file currently being lexed.
std::shared_ptr<fs::path>  sourcefile()
{
  if (!filename_stack.empty()) return filename_stack.back();

  return parser_sourcefile;
}

bool lexer_is_main_file()
{
  return loc_stack.empty();
}

/*
  Rules for include <path/file>
  1) include <sourcepath/path/file>
  2) include <librarydir/path/file>

  Globals used: filepath, sourcefile, filename
 */
void includefile(const Location& loc)
{
  fs::path localpath = fs::path(filepath) / filename;
  fs::path fullpath = find_valid_path(sourcefile()->parent_path(), localpath, &openfilenames);
  if (!fullpath.empty()) {
    rootmodule->registerInclude(localpath.generic_string(), fullpath.generic_string(), lexer_is_main_file() ? loc : Location::NONE);
  }
  else {
    rootmodule->registerInclude(localpath.generic_string(), localpath.generic_string(), Location::NONE);
    LOG(message_group::Warning,Location::NONE,"","Can't open include file '%1$s'.",localpath.generic_string());
    return;
  };

  std::string fullname = fullpath.generic_string();

  filepath.clear();
  filename_stack.push_back(std::make_shared<fs::path>(fullpath));

  handle_dep(fullname);

  yyin = fopen(fullname.c_str(), "r");
  if (!yyin) {
    LOG(message_group::Warning,Location::NONE,"","Can't open include file '%1$s'.",localpath.generic_string());
    filename_stack.pop_back();
    return;
  }

  loc_stack.push_back(parserlloc);
  LOCATION_INIT(parserlloc);  
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
	for (auto f : openfiles) fclose(f);
	openfiles.clear();
	openfilenames.clear();
	filename_stack.clear();
	loc_stack.clear();
}

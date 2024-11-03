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

%expect 0

%{

#include <sys/types.h>
#include <sys/stat.h>
#ifdef _MSC_VER
#define strdup _strdup
#else
#include <unistd.h>
#endif

#include "core/SourceFile.h"
#include "core/UserModule.h"
#include "core/ModuleInstantiation.h"
#include "core/Assignment.h"
#include "core/Expression.h"
#include "core/function.h"
#include "io/fileutils.h"
#include "utils/printutils.h"
#include <memory>
#include <sstream>
#include <stack>
#include <filesystem>

namespace fs = std::filesystem;

#define YYMAXDEPTH 20000
#define LOC(loc) Location(loc.first_line, loc.first_column, loc.last_line, loc.last_column, sourcefile())
#ifdef DEBUG
static Location debug_location(const std::string& info, const struct YYLTYPE& loc);
#define LOCD(str, loc) debug_location(str, loc)
#else
#define LOCD(str, loc) LOC(loc)
#endif

int parser_error_pos = -1;

int parserlex(void);
void yyerror(char const *s);

int lexerget_lineno(void);
bool lexer_is_main_file();
std::shared_ptr<fs::path> sourcefile(void);
void lexer_set_parser_sourcefile(const fs::path& path);
int lexerlex_destroy(void);
int lexerlex(void);
static void handle_assignment(const std::string token, Expression *expr, const Location loc);

std::stack<LocalScope *> scope_stack;
SourceFile *rootfile;

extern void lexerdestroy();
extern FILE *lexerin;
const char *parser_input_buffer;
static fs::path mainFilePath;
static bool parsingMainFile;

bool fileEnded=false;
%}

%initial-action
{
  @$.first_line = 1;
  @$.first_column = 1;
  @$.last_column = 1;
  @$.last_line = 1;
};

%union {
  char *text;
  double number;
  class Expression *expr;
  class Vector *vec;
  class ModuleInstantiation *inst;
  class IfElseModuleInstantiation *ifelse;
  class Assignment *arg;
  AssignmentList *args;
}

%token TOK_ERROR

%token TOK_EOT

%token TOK_MODULE
%token TOK_FUNCTION
%token TOK_IF
%token TOK_ELSE
%token TOK_FOR
%token TOK_LET
%token TOK_ASSERT
%token TOK_ECHO
%token TOK_EACH

%token <text> TOK_ID
%token <text> TOK_STRING
%token <text> TOK_USE
%token <number> TOK_NUMBER

%token TOK_TRUE
%token TOK_FALSE
%token TOK_UNDEF

%token LE GE EQ NEQ AND OR

%nonassoc NO_ELSE
%nonassoc TOK_ELSE

%type <expr> expr
%type <expr> call
%type <expr> logic_or
%type <expr> logic_and
%type <expr> equality
%type <expr> comparison
%type <expr> addition
%type <expr> multiplication
%type <expr> exponent
%type <expr> unary
%type <expr> primary
%type <vec> vector_elements
%type <expr> list_comprehension_elements
%type <expr> list_comprehension_elements_p
%type <expr> vector_element
%type <expr> expr_or_empty

%type <inst> module_instantiation
%type <ifelse> if_statement
%type <ifelse> ifelse_statement
%type <inst> single_module_instantiation

%type <args> arguments
%type <args> argument_list
%type <args> parameters
%type <args> parameter_list

%type <arg> argument
%type <arg> parameter
%type <text> module_id

%debug
%locations

%%

input
        : /* empty */
        | input
          TOK_USE
            {
              rootfile->registerUse(std::string($2), lexer_is_main_file() && parsingMainFile ? LOC(@2) : Location::NONE);
              free($2);
            }
        | input statement
        ;

statement
        : ';'
        | '{' inner_input '}'
        | module_instantiation
            {
              if ($1) scope_stack.top()->addModuleInst(std::shared_ptr<ModuleInstantiation>($1));
            }
        | assignment
        | TOK_MODULE TOK_ID '(' parameters ')'
            {
              UserModule *newmodule = new UserModule($2, LOCD("module", @$));
              newmodule->parameters = *$4;
              auto top = scope_stack.top();
              scope_stack.push(&newmodule->body);
              top->addModule(std::shared_ptr<UserModule>(newmodule));
              free($2);
              delete $4;
            }
          statement
            {
                scope_stack.pop();
            }
        | TOK_FUNCTION TOK_ID '(' parameters ')' '=' expr ';'
            {
              scope_stack.top()->addFunction(
                std::make_shared<UserFunction>($2, *$4, std::shared_ptr<Expression>($7), LOCD("function", @$))
              );
              free($2);
              delete $4;
            }
        | TOK_EOT
            {
                fileEnded=true;
            }
        ;

inner_input
        : /* empty */
        | inner_input statement
        ;

assignment
        : TOK_ID '=' expr ';'
            {
                handle_assignment($1, $3, LOCD("assignment", @$));
                free($1);
            }
        ;

module_instantiation
        : '!' module_instantiation
            {
                $$ = $2;
                if ($$) $$->tag_root = true;
            }
        | '#' module_instantiation
            {
                $$ = $2;
                if ($$) $$->tag_highlight = true;
            }
        | '%' module_instantiation
            {
                $$ = $2;
                if ($$) $$->tag_background = true;
            }
        | '*' module_instantiation
            {
                delete $2;
                $$ = NULL;
            }
        | single_module_instantiation
            {
                $<inst>$ = $1;
                scope_stack.push(&$1->scope);
            }
          child_statement
            {
                scope_stack.pop();
                $$ = $<inst>2;
            }
        | ifelse_statement
            {
                $$ = $1;
            }
        ;

ifelse_statement
        : if_statement %prec NO_ELSE
            {
                $$ = $1;
            }
        | if_statement TOK_ELSE
            {
                scope_stack.push($1->makeElseScope());
            }
          child_statement
            {
                scope_stack.pop();
                $$ = $1;
            }
        ;

if_statement
        : TOK_IF '(' expr ')'
            {
                $<ifelse>$ = new IfElseModuleInstantiation(std::shared_ptr<Expression>($3), LOCD("if", @$));
                scope_stack.push(&$<ifelse>$->scope);
            }
          child_statement
            {
                scope_stack.pop();
                $$ = $<ifelse>5;
            }
        ;

child_statements
        : /* empty */
        | child_statements child_statement
        | child_statements assignment
        ;

child_statement
        : ';'
        | '{' child_statements '}'
        | module_instantiation
            {
                if ($1) scope_stack.top()->addModuleInst(std::shared_ptr<ModuleInstantiation>($1));
            }
        ;

// "for", "let" and "each" are valid module identifiers
module_id
        : TOK_ID  { $$ = $1; }
        | TOK_FOR { $$ = strdup("for"); }
        | TOK_LET { $$ = strdup("let"); }
        | TOK_ASSERT { $$ = strdup("assert"); }
        | TOK_ECHO { $$ = strdup("echo"); }
        | TOK_EACH { $$ = strdup("each"); }
        ;

single_module_instantiation
        : module_id '(' arguments ')'
            {
                $$ = new ModuleInstantiation($1, *$3, LOCD("modulecall", @$));
                free($1);
                delete $3;
            }
        ;

expr
        : logic_or
        | TOK_FUNCTION '(' parameters ')' expr %prec NO_ELSE
            {
              $$ = new FunctionDefinition($5, *$3, LOCD("anonfunc", @$));
              delete $3;
            }
        | logic_or '?' expr ':' expr
            {
              $$ = new TernaryOp($1, $3, $5, LOCD("ternary", @$));
            }
        | TOK_LET '(' arguments ')' expr
            {
              $$ = FunctionCall::create("let", *$3, $5, LOCD("let", @$));
              delete $3;
            }
        | TOK_ASSERT '(' arguments ')' expr_or_empty
            {
              $$ = FunctionCall::create("assert", *$3, $5, LOCD("assert", @$));
              delete $3;
            }
        | TOK_ECHO '(' arguments ')' expr_or_empty
            {
              $$ = FunctionCall::create("echo", *$3, $5, LOCD("echo", @$));
              delete $3;
            }
        ;

logic_or
        : logic_and
        | logic_or OR logic_and
            {
              $$ = new BinaryOp($1, BinaryOp::Op::LogicalOr, $3, LOCD("or", @$));
            }
		;

logic_and
        : equality
        | logic_and AND equality
            {
              $$ = new BinaryOp($1, BinaryOp::Op::LogicalAnd, $3, LOCD("and", @$));
            }
		;

equality
        : comparison
        | equality EQ comparison
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Equal, $3, LOCD("equal", @$));
            }
        | equality NEQ comparison
            {
              $$ = new BinaryOp($1, BinaryOp::Op::NotEqual, $3, LOCD("notequal", @$));
            }
		;

comparison
        : addition
        | comparison '>' addition
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Greater, $3, LOCD("greater", @$));
            }
        | comparison GE addition
            {
              $$ = new BinaryOp($1, BinaryOp::Op::GreaterEqual, $3, LOCD("greaterequal", @$));
            }
        | comparison '<' addition
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Less, $3, LOCD("less", @$));
            }
        | comparison LE addition
            {
              $$ = new BinaryOp($1, BinaryOp::Op::LessEqual, $3, LOCD("lessequal", @$));
            }
		;

addition
        : multiplication
        | addition '+' multiplication
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Plus, $3, LOCD("addition", @$));
            }
        | addition '-' multiplication
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Minus, $3, LOCD("subtraction", @$));
            }
		;

multiplication
        : unary
        | multiplication '*' unary
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Multiply, $3, LOCD("multiply", @$));
            }
        | multiplication '/' unary
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Divide, $3, LOCD("divide", @$));
            }
        | multiplication '%' unary
            {
              $$ = new BinaryOp($1, BinaryOp::Op::Modulo, $3, LOCD("modulo", @$));
            }
		;


unary
        : exponent
        | '+' unary
            {
                $$ = $2;
            }
        | '-' unary
            {
              Literal* argument = dynamic_cast<Literal*>($2);
              if (argument && argument->isDouble()) {
                double value = argument->toDouble();
                delete $2;
                $$ = new Literal(-value, LOCD("literal", @$));
              } else {
                $$ = new UnaryOp(UnaryOp::Op::Negate, $2, LOCD("negate", @$));
              }
            }
        | '!' unary
            {
              $$ = new UnaryOp(UnaryOp::Op::Not, $2, LOCD("not", @$));
            }
		;

exponent
       : call
       | call '^' unary
           {
              $$ = new BinaryOp($1, BinaryOp::Op::Exponent, $3, LOCD("exponent", @$));
           }
       ;

call
        : primary
        | call '(' arguments ')'
            {
              $$ = new FunctionCall($1, *$3, LOCD("functioncall", @$));
              delete $3;
            }
        | call '[' expr ']'
            {
              $$ = new ArrayLookup($1, $3, LOCD("index", @$));
            }
        | call '.' TOK_ID
            {
              $$ = new MemberLookup($1, $3, LOCD("member", @$));
              free($3);
            }
		;

primary
        : TOK_TRUE
            {
              $$ = new Literal(true, LOCD("literal", @$));
            }
        | TOK_FALSE
            {
              $$ = new Literal(false, LOCD("literal", @$));
            }
        | TOK_UNDEF
            {
              $$ = new Literal(LOCD("literal", @$));
            }
        | TOK_NUMBER
            {
              $$ = new Literal($1, LOCD("literal", @$));
            }
        | TOK_STRING
            {
              $$ = new Literal(std::string($1), LOCD("string", @$));
              free($1);
            }
        | TOK_ID
            {
              $$ = new Lookup($1, LOCD("variable", @$));
              free($1);
            }
        | '(' expr ')'
            {
              $$ = $2;
            }
        | '[' expr ':' expr ']'
            {
              $$ = new Range($2, $4, LOCD("range", @$));
            }
        | '[' expr ':' expr ':' expr ']'
            {
              $$ = new Range($2, $4, $6, LOCD("range", @$));
            }
        | '[' ']'
            {
              $$ = new Vector(LOCD("vector", @$));
            }
        | '[' vector_elements optional_trailing_comma ']'
            {
              $$ = $2;
            }
		;

expr_or_empty
        : /* empty */
            {
              $$ = NULL;
            }
        | expr
            {
              $$ = $1;
            }
        ;

/* The last set element may not be a "let" (as that would instead
   be parsed as an expression) */
list_comprehension_elements
        : TOK_LET '(' arguments ')' list_comprehension_elements_p
            {
              $$ = new LcLet(*$3, $5, LOCD("lclet", @$));
              delete $3;
            }
        | TOK_EACH vector_element
            {
              $$ = new LcEach($2, LOCD("lceach", @$));
            }
        | TOK_FOR '(' arguments ')' vector_element
            {
              $$ = new LcFor(*$3, $5, LOCD("lcfor", @$));
              delete $3;
            }
        | TOK_FOR '(' arguments ';' expr ';' arguments ')' vector_element
            {
              $$ = new LcForC(*$3, *$7, $5, $9, LOCD("lcforc", @$));
              delete $3;
              delete $7;
            }
        | TOK_IF '(' expr ')' vector_element %prec NO_ELSE
            {
              $$ = new LcIf($3, $5, 0, LOCD("lcif", @$));
            }
        | TOK_IF '(' expr ')' vector_element TOK_ELSE vector_element
            {
              $$ = new LcIf($3, $5, $7, LOCD("lcifelse", @$));
            }
        ;

// list_comprehension_elements with optional parenthesis
list_comprehension_elements_p
        : list_comprehension_elements
        | '(' list_comprehension_elements ')'
            {
                $$ = $2;
            }
        ;

optional_trailing_comma
        : /* empty */
        | ','
        ;

vector_elements
        : vector_element
            {
              $$ = new Vector(LOCD("vector", @$));
              $$->emplace_back($1);
            }
        | vector_elements ',' vector_element
            {
              $$ = $1;
              $$->emplace_back($3);
            }
        ;

vector_element
        : list_comprehension_elements_p
        | expr
        ;

parameters
        : /* empty */
            {
                $$ = new AssignmentList();
            }
        | parameter_list optional_trailing_comma
        ;

parameter_list
        : parameter
            {
                $$ = new AssignmentList();
                $$->emplace_back($1);
            }
        | parameter_list ',' parameter
            {
                $$ = $1;
                $$->emplace_back($3);
            }
        ;

parameter
        : TOK_ID
            {
                $$ = new Assignment($1, LOCD("assignment", @$));
                free($1);
            }
        | TOK_ID '=' expr
            {
              $$ = new Assignment($1, std::shared_ptr<Expression>($3), LOCD("assignment", @$));
                free($1);
            }
        ;

arguments
        : /* empty */
            {
                $$ = new AssignmentList();
            }
        | argument_list optional_trailing_comma
        ;

argument_list
        : argument
            {
                $$ = new AssignmentList();
                $$->emplace_back($1);
            }
        | argument_list ',' argument
            {
                $$ = $1;
                $$->emplace_back($3);
            }
        ;

argument
        : expr
            {
                $$ = new Assignment("", std::shared_ptr<Expression>($1), LOCD("argumentcall", @$));
            }
        | TOK_ID '=' expr
            {
                $$ = new Assignment($1, std::shared_ptr<Expression>($3), LOCD("argumentcall", @$));
                free($1);
            }
        ;

%%

int parserlex(void)
{
  return lexerlex();
}

void yyerror (char const *s)
{
  // FIXME: We leak memory on parser errors...
	Location loc = Location(lexerget_lineno(), -1, -1, -1, sourcefile());
	LOG(message_group::Error, loc, "", "Parser error: %1$s", s);
}

#ifdef DEBUG
static Location debug_location(const std::string& info, const YYLTYPE& loc)
{
	auto location = LOC(loc);
	PRINTDB("%3d, %3d - %3d, %3d | %s", loc.first_line % loc.first_column % loc.last_line % loc.last_column % info);
	return location;
}
#endif

static void warn_reassignment(const Location& loc, const std::shared_ptr<Assignment>& assignment, const fs::path& path)
{
	LOG(message_group::Warning,
			loc,
			path.parent_path().generic_string(),
			"%1$s was assigned on line %2$i but was overwritten",
			assignment->getName(),
			assignment->location().firstLine());

}

static void warn_reassignment(const Location& loc, const std::shared_ptr<Assignment>& assignment, const fs::path& path1, const fs::path& path2)
{
	LOG(message_group::Warning,
			loc,
			path1.parent_path().generic_string(),
			"%1$s was assigned on line %2$i of %3$s but was overwritten",
			assignment->getName(),
			assignment->location().firstLine(),
			path2);
}

void handle_assignment(const std::string token, Expression *expr, const Location loc)
{
	bool found = false;
	for (auto &assignment : scope_stack.top()->assignments) {
		if (assignment->getName() == token) {
			auto mainFile = mainFilePath.string();
			auto prevFile = assignment->location().fileName();
			auto currFile = loc.fileName();

			const auto uncPathCurr = fs_uncomplete(currFile, mainFilePath.parent_path());
			const auto uncPathPrev = fs_uncomplete(prevFile, mainFilePath.parent_path());
			if (fileEnded) {
				//assignments via commandline
			} else if (prevFile == mainFile && currFile == mainFile) {
				//both assignments in the mainFile
				warn_reassignment(loc, assignment, mainFilePath);
			} else if (uncPathCurr == uncPathPrev) {
				//assignment overwritten within the same file
				//the line number being equal happens, when a file is included multiple times
				if (assignment->location().firstLine() != loc.firstLine()) {
					warn_reassignment(loc, assignment, mainFilePath, uncPathPrev);
				}
			} else if (prevFile == mainFile && currFile != mainFile) {
				//assignment from the mainFile overwritten by an include
				warn_reassignment(loc, assignment, mainFilePath, uncPathPrev);
			}
			assignment->setExpr(std::shared_ptr<Expression>(expr));
			assignment->setLocationOfOverwrite(loc);
			found = true;
			break;
		}
	}
	if (!found) {
		scope_stack.top()->addAssignment(assignment(token, std::shared_ptr<Expression>(expr), loc));
	}
}

bool parse(SourceFile *&file, const std::string& text, const std::string &filename, const std::string &mainFile, int debug)
{
  fs::path filepath;
  try {
    filepath = filename.empty() ? fs::current_path() : fs::absolute(fs::path{filename});
    mainFilePath = mainFile.empty() ? fs::current_path() : fs::absolute(fs::path{mainFile});
  } catch (...) {
    // yyerror tries to print the file path, which throws again, and we can't do that
    LOG(message_group::Error, "Parser error: file access denied");
    return false;
  }

  parsingMainFile = mainFilePath == filepath;
  fs::path parser_sourcefile = fs::path(filepath).generic_string();
  lexer_set_parser_sourcefile(parser_sourcefile);

  lexerin = NULL;
  parser_error_pos = -1;
  parser_input_buffer = text.c_str();
  fileEnded = false;

  rootfile = new SourceFile(parser_sourcefile.parent_path().string(), parser_sourcefile.filename().string());
  scope_stack.push(&rootfile->scope);
  //        PRINTB_NOCACHE("New module: %s %p", "root" % rootfile);

  parserdebug = debug;
  int parserretval = -1;
  try{
     parserretval = parserparse();
  }catch (const HardWarningException &e) {
    yyerror("stop on first warning");
  }

  lexerdestroy();
  lexerlex_destroy();

  file = rootfile;
  if (parserretval != 0) return false;

  parser_error_pos = -1;
  parser_input_buffer = nullptr;
  scope_stack.pop();

  return true;
}

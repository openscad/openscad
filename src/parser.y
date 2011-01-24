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

%expect 1 /* Expect 1 shift/reduce conflict for ifelse_statement - "dangling else problem" */

%{

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "module.h"
#include "expression.h"
#include "value.h"
#include "function.h"
#include "printutils.h"

int parser_error_pos = -1;

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
	class Value *value;
	class Expression *expr;
	class ModuleInstantiation *inst;
	class IfElseModuleInstantiation *ifelse;
	class ArgContainer *arg;
	class ArgsContainer *args;
}

%token TOK_MODULE
%token TOK_FUNCTION
%token TOK_IF
%token TOK_ELSE

%token <text> TOK_ID
%token <text> TOK_STRING
%token <text> TOK_USE
%token <number> TOK_NUMBER

%token TOK_TRUE
%token TOK_FALSE
%token TOK_UNDEF

%token LE GE EQ NE AND OR

%right '?' ':'

%left OR
%left AND

%left '<' LE GE '>'
%left EQ NE

%left '!' '+' '-'
%left '*' '/' '%'
%left '[' ']'
%left '.'

%type <expr> expr
%type <expr> vector_expr

%type <inst> module_instantiation
%type <ifelse> if_statement
%type <ifelse> ifelse_statement
%type <inst> children_instantiation
%type <inst> module_instantiation_list
%type <inst> single_module_instantiation

%type <args> arguments_call
%type <args> arguments_decl

%type <arg> argument_call
%type <arg> argument_decl

%debug

%%

input: 
	/* empty */ |
	TOK_USE { module->usedlibs[$1] = NULL; } input |
	statement input ;

inner_input: 
	/* empty */ |
	statement inner_input ;

statement:
	';' |
	'{' inner_input '}' |
	module_instantiation {
		if ($1) {
			module->children.append($1);
		} else {
			delete $1;
		}
	} |
	TOK_ID '=' expr ';' {
		bool add_new_assignment = true;
		for (int i = 0; i < module->assignments_var.size(); i++) {
			if (module->assignments_var[i] != QString($1))
				continue;
			delete module->assignments_expr[i];
			module->assignments_expr[i] = $3;
			add_new_assignment = false;
		}
		if (add_new_assignment) {
			module->assignments_var.append($1);
			module->assignments_expr.append($3);
			free($1);
		}
	} |
	TOK_MODULE TOK_ID '(' arguments_decl optional_commas ')' {
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
	TOK_FUNCTION TOK_ID '(' arguments_decl optional_commas ')' '=' expr {
		Function *func = new Function();
		func->argnames = $4->argnames;
		func->argexpr = $4->argexpr;
		func->expr = $8;
		module->functions[$2] = func;
		free($2);
		delete $4;
	} ';' ;

/* Will return a dummy parent node with zero or more children */
children_instantiation:
	module_instantiation {
		$$ = new ModuleInstantiation();
		if ($1) {
			$$->children.append($1);
		} else {
			delete $1;
		}
	} |
	'{' module_instantiation_list '}' {
		$$ = $2;
	} ;

if_statement:
	TOK_IF '(' expr ')' children_instantiation {
		$$ = new IfElseModuleInstantiation();
		$$->modname = "if";
		$$->argnames.append(QString());
		$$->argexpr.append($3);

		if ($$) {
			$$->children = $5->children;
		} else {
			for (int i = 0; i < $5->children.count(); i++)
				delete $5->children[i];
		}
		$5->children.clear();
		delete $5;
	} ;

ifelse_statement:
	if_statement {
		$$ = $1;
	} |
	if_statement TOK_ELSE children_instantiation {
		$$ = $1;
		if ($$) {
			$$->else_children = $3->children;
		} else {
			for (int i = 0; i < $3->children.count(); i++)
				delete $3->children[i];
		}
		$3->children.clear();
		delete $3;
	} ;

module_instantiation:
	single_module_instantiation ';' {
		$$ = $1;
	} |
	single_module_instantiation children_instantiation {
		$$ = $1;
		if ($$) {
			$$->children = $2->children;
		} else {
			for (int i = 0; i < $2->children.count(); i++)
				delete $2->children[i];
		}
		$2->children.clear();
		delete $2;
	} |
	ifelse_statement {
		$$ = $1;
	} ;

module_instantiation_list:
	/* empty */ {
		$$ = new ModuleInstantiation();
	} |
	module_instantiation_list module_instantiation {
		$$ = $1;
		if ($$) {
			if ($2)
				$$->children.append($2);
		} else {
			delete $2;
		}
	} ;

single_module_instantiation:
	TOK_ID '(' arguments_call ')' {
		$$ = new ModuleInstantiation();
		$$->modname = QString($1);
		$$->argnames = $3->argnames;
		$$->argexpr = $3->argexpr;
		free($1);
		delete $3;
	} |
	TOK_ID ':' single_module_instantiation {
		$$ = $3;
		if ($$)
			$$->label = QString($1);
		free($1);
	} |
	'!' single_module_instantiation {
		$$ = $2;
		if ($$)
			$$->tag_root = true;
	} |
	'#' single_module_instantiation {
		$$ = $2;
		if ($$)
			$$->tag_highlight = true;
	} |
	'%' single_module_instantiation {
		$$ = $2;
		if ($$)
			$$->tag_background = true;
	} |
	'*' single_module_instantiation {
		delete $2;
		$$ = NULL;
	};

expr:
	TOK_TRUE {
		$$ = new Expression();
		$$->type = "C";
		$$->const_value = new Value(true);
	} |
	TOK_FALSE {
		$$ = new Expression();
		$$->type = "C";
		$$->const_value = new Value(false);
	} |
	TOK_UNDEF {
		$$ = new Expression();
		$$->type = "C";
		$$->const_value = new Value();
	} |
	TOK_ID {
		$$ = new Expression();
		$$->type = "L";
		$$->var_name = QString($1);
		free($1);
	} |
	expr '.' TOK_ID {
		$$ = new Expression();
		$$->type = "N";
		$$->children.append($1);
		$$->var_name = QString($3);
		free($3);
	} |
	TOK_STRING {
		$$ = new Expression();
		$$->type = "C";
		$$->const_value = new Value(QString($1));
		free($1);
	} |
	TOK_NUMBER {
		$$ = new Expression();
		$$->type = "C";
		$$->const_value = new Value($1);
	} |
	'[' expr ':' expr ']' {
		Expression *e_one = new Expression();
		e_one->type = "C";
		e_one->const_value = new Value(1.0);
		$$ = new Expression();
		$$->type = "R";
		$$->children.append($2);
		$$->children.append(e_one);
		$$->children.append($4);
	} |
	'[' expr ':' expr ':' expr ']' {
		$$ = new Expression();
		$$->type = "R";
		$$->children.append($2);
		$$->children.append($4);
		$$->children.append($6);
	} |
	'[' optional_commas ']' {
		$$ = new Expression();
		$$->type = "C";
		$$->const_value = new Value();
		$$->const_value->type = Value::VECTOR;
	} |
	'[' vector_expr optional_commas ']' {
		$$ = $2;
	} |
	expr '*' expr {
		$$ = new Expression();
		$$->type = "*";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '/' expr {
		$$ = new Expression();
		$$->type = "/";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '%' expr {
		$$ = new Expression();
		$$->type = "%";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '+' expr {
		$$ = new Expression();
		$$->type = "+";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '-' expr {
		$$ = new Expression();
		$$->type = "-";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '<' expr {
		$$ = new Expression();
		$$->type = "<";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr LE expr {
		$$ = new Expression();
		$$->type = "<=";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr EQ expr {
		$$ = new Expression();
		$$->type = "==";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr NE expr {
		$$ = new Expression();
		$$->type = "!=";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr GE expr {
		$$ = new Expression();
		$$->type = ">=";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr '>' expr {
		$$ = new Expression();
		$$->type = ">";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr AND expr {
		$$ = new Expression();
		$$->type = "&&";
		$$->children.append($1);
		$$->children.append($3);
	} |
	expr OR expr {
		$$ = new Expression();
		$$->type = "||";
		$$->children.append($1);
		$$->children.append($3);
	} |
	'+' expr {
		$$ = $2;
	} |
	'-' expr {
		$$ = new Expression();
		$$->type = "I";
		$$->children.append($2);
	} |
	'!' expr {
		$$ = new Expression();
		$$->type = "!";
		$$->children.append($2);
	} |
	'(' expr ')' {
		$$ = $2;
	} |
	expr '?' expr ':' expr {
		$$ = new Expression();
		$$->type = "?:";
		$$->children.append($1);
		$$->children.append($3);
		$$->children.append($5);
	} |
	expr '[' expr ']' {
		$$ = new Expression();
		$$->type = "[]";
		$$->children.append($1);
		$$->children.append($3);
	} |
	TOK_ID '(' arguments_call ')' {
		$$ = new Expression();
		$$->type = "F";
		$$->call_funcname = QString($1);
		$$->call_argnames = $3->argnames;
		$$->children = $3->argexpr;
		free($1);
		delete $3;
	} ;

optional_commas:
	',' optional_commas | ;

vector_expr:
	expr {
		$$ = new Expression();
		$$->type = 'V';
		$$->children.append($1);
	} |
	vector_expr ',' optional_commas expr {
		$$ = $1;
		$$->children.append($4);
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
	arguments_decl ',' optional_commas argument_decl {
		$$ = $1;
		$$->argnames.append($4->argname);
		$$->argexpr.append($4->argexpr);
		delete $4;
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
	arguments_call ',' optional_commas argument_call {
		$$ = $1;
		$$->argnames.append($4->argname);
		$$->argexpr.append($4->argexpr);
		delete $4;
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
	// FIXME: We leak memory on parser errors...
	PRINTF("Parser error in line %d: %s\n", lexerget_lineno(), s);
	module = NULL;
}

extern FILE *lexerin;
extern const char *parser_input_buffer;
const char *parser_input_buffer;
const char *parser_source_path;

AbstractModule *parse(const char *text, const char *path, int debug)
{
	lexerin = NULL;
	parser_error_pos = -1;
	parser_input_buffer = text;
	parser_source_path = path;

	module_stack.clear();
	module = new Module();

	parserdebug = debug;
	parserparse();

	lexerlex_destroy();

	if (!module)
		return NULL;

	QHashIterator<QString, Module*> i(module->usedlibs);
	while (i.hasNext()) {
		i.next();
		module->usedlibs[i.key()] = Module::compile_library(i.key());
		if (!module->usedlibs[i.key()]) {
			PRINTF("WARNING: Failed to compile library `%s'.", i.key().toUtf8().data());
			module->usedlibs.remove(i.key());
		}
	}

	parser_error_pos = -1;
	return module;
}

QHash<QString, Module::libs_cache_ent> Module::libs_cache;

Module *Module::compile_library(QString filename)
{
	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	stat(filename.toAscii().data(), &st);

	QString cache_id;
	cache_id.sprintf("%x.%x", (int)st.st_mtime, (int)st.st_size);

	if (libs_cache.contains(filename) && libs_cache[filename].cache_id == cache_id) {
		PRINT(libs_cache[filename].msg);
		return &(*libs_cache[filename].mod);
	}

	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		PRINTF("WARNING: Can't open library file `%s'.", filename.toUtf8().data());
		return NULL;
	}
	QString text = QTextStream(&f).readAll();

	print_messages_push();

	PRINTF("Compiling library `%s'.", filename.toUtf8().data());
	libs_cache_ent e = { NULL, cache_id, QString("WARNING: Library `%1' tries to recursively use itself!").arg(filename) };
	if (libs_cache.contains(filename))
		delete libs_cache[filename].mod;
	libs_cache[filename] = e;

	Module *backup_mod = module;
	Module *lib_mod = dynamic_cast<Module*>(parse(text.toLocal8Bit(), QFileInfo(filename).absoluteDir().absolutePath().toLocal8Bit(), 0));
	module = backup_mod;

	if (lib_mod) {
		libs_cache[filename].mod = lib_mod;
		libs_cache[filename].msg = print_messages_stack.last();
	} else {
		libs_cache.remove(filename);
	}

	print_messages_pop();

	return lib_mod;
}


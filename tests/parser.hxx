/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_PARSER_USERS_KINTEL_CODE_OPENSCAD_OPENSCAD_TESTS_PARSER_HXX_INCLUDED
# define YY_PARSER_USERS_KINTEL_CODE_OPENSCAD_OPENSCAD_TESTS_PARSER_HXX_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int parserdebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOK_ERROR = 258,
    TOK_MODULE = 259,
    TOK_FUNCTION = 260,
    TOK_IF = 261,
    TOK_ELSE = 262,
    TOK_FOR = 263,
    TOK_LET = 264,
    TOK_ASSERT = 265,
    TOK_ECHO = 266,
    TOK_EACH = 267,
    TOK_ID = 268,
    TOK_STRING = 269,
    TOK_USE = 270,
    TOK_NUMBER = 271,
    TOK_TRUE = 272,
    TOK_FALSE = 273,
    TOK_UNDEF = 274,
    LE = 275,
    GE = 276,
    EQ = 277,
    NE = 278,
    AND = 279,
    OR = 280,
    LET = 281,
    LOW_PRIO_RIGHT = 282,
    LOW_PRIO_LEFT = 283,
    HIGH_PRIO_RIGHT = 284,
    HIGH_PRIO_LEFT = 285
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 75 "../src/parser.y" /* yacc.c:1909  */

  char *text;
  double number;
  class Value *value;
  class Expression *expr;
  class Vector *vec;
  class ModuleInstantiation *inst;
  class IfElseModuleInstantiation *ifelse;
  class Assignment *arg;
  AssignmentList *args;

#line 97 "/Users/kintel/code/OpenSCAD/openscad/tests/parser.hxx" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE parserlval;
extern YYLTYPE parserlloc;
int parserparse (void);

#endif /* !YY_PARSER_USERS_KINTEL_CODE_OPENSCAD_OPENSCAD_TESTS_PARSER_HXX_INCLUDED  */

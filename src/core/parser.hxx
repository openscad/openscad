/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_PARSER_HOME_GSOHLER_GIT_PYTHONSCAD_BUILD_OBJECTS_PARSER_HXX_INCLUDED
# define YY_PARSER_HOME_GSOHLER_GIT_PYTHONSCAD_BUILD_OBJECTS_PARSER_HXX_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int parserdebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    TOK_ERROR = 258,               /* TOK_ERROR  */
    TOK_EOT = 259,                 /* TOK_EOT  */
    TOK_MODULE = 260,              /* TOK_MODULE  */
    TOK_FUNCTION = 261,            /* TOK_FUNCTION  */
    TOK_IF = 262,                  /* TOK_IF  */
    TOK_ELSE = 263,                /* TOK_ELSE  */
    TOK_FOR = 264,                 /* TOK_FOR  */
    TOK_LET = 265,                 /* TOK_LET  */
    TOK_ASSERT = 266,              /* TOK_ASSERT  */
    TOK_ECHO = 267,                /* TOK_ECHO  */
    TOK_EACH = 268,                /* TOK_EACH  */
    TOK_ID = 269,                  /* TOK_ID  */
    TOK_STRING = 270,              /* TOK_STRING  */
    TOK_USE = 271,                 /* TOK_USE  */
    TOK_NUMBER = 272,              /* TOK_NUMBER  */
    TOK_TRUE = 273,                /* TOK_TRUE  */
    TOK_FALSE = 274,               /* TOK_FALSE  */
    TOK_UNDEF = 275,               /* TOK_UNDEF  */
    LE = 276,                      /* LE  */
    GE = 277,                      /* GE  */
    EQ = 278,                      /* EQ  */
    NEQ = 279,                     /* NEQ  */
    AND = 280,                     /* AND  */
    OR = 281,                      /* OR  */
    NO_ELSE = 282                  /* NO_ELSE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 96 "src/core/parser.y"

  char *text;
  double number;
  class Expression *expr;
  class Vector *vec;
  class ModuleInstantiation *inst;
  class IfElseModuleInstantiation *ifelse;
  class Assignment *arg;
  AssignmentList *args;

#line 102 "/home/gsohler/git/pythonscad/build/objects/parser.hxx"

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


#endif /* !YY_PARSER_HOME_GSOHLER_GIT_PYTHONSCAD_BUILD_OBJECTS_PARSER_HXX_INCLUDED  */

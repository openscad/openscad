/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_ERROR = 258,
     TOK_EOT = 259,
     TOK_MODULE = 260,
     TOK_FUNCTION = 261,
     TOK_IF = 262,
     TOK_ELSE = 263,
     TOK_FOR = 264,
     TOK_LET = 265,
     TOK_ASSERT = 266,
     TOK_ECHO = 267,
     TOK_EACH = 268,
     TOK_ID = 269,
     TOK_STRING = 270,
     TOK_USE = 271,
     TOK_NUMBER = 272,
     TOK_TRUE = 273,
     TOK_FALSE = 274,
     TOK_UNDEF = 275,
     LE = 276,
     GE = 277,
     EQ = 278,
     NE = 279,
     AND = 280,
     OR = 281,
     LET = 282,
     LOW_PRIO_RIGHT = 283,
     LOW_PRIO_LEFT = 284,
     HIGH_PRIO_RIGHT = 285,
     HIGH_PRIO_LEFT = 286
   };
#endif
/* Tokens.  */
#define TOK_ERROR 258
#define TOK_EOT 259
#define TOK_MODULE 260
#define TOK_FUNCTION 261
#define TOK_IF 262
#define TOK_ELSE 263
#define TOK_FOR 264
#define TOK_LET 265
#define TOK_ASSERT 266
#define TOK_ECHO 267
#define TOK_EACH 268
#define TOK_ID 269
#define TOK_STRING 270
#define TOK_USE 271
#define TOK_NUMBER 272
#define TOK_TRUE 273
#define TOK_FALSE 274
#define TOK_UNDEF 275
#define LE 276
#define GE 277
#define EQ 278
#define NE 279
#define AND 280
#define OR 281
#define LET 282
#define LOW_PRIO_RIGHT 283
#define LOW_PRIO_LEFT 284
#define HIGH_PRIO_RIGHT 285
#define HIGH_PRIO_LEFT 286




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 78 "../src/parser.y"
{
  char *text;
  double number;
  class Value *value;
  class Expression *expr;
  class Vector *vec;
  class ModuleInstantiation *inst;
  class IfElseModuleInstantiation *ifelse;
  class Assignment *arg;
  AssignmentList *args;
}
/* Line 1529 of yacc.c.  */
#line 123 "/Users/kintel/code/OpenSCAD/openscad/tests/parser.hxx"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE parserlval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE parserlloc;

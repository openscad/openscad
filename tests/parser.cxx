/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse parserparse
#define yylex   parserlex
#define yyerror parsererror
#define yylval  parserlval
#define yychar  parserchar
#define yydebug parserdebug
#define yynerrs parsernerrs
#define yylloc parserlloc

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




/* Copy the first part of user declarations.  */
#line 29 "../src/parser.y"


#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "FileModule.h"
#include "UserModule.h"
#include "ModuleInstantiation.h"
#include "Assignment.h"
#include "expression.h"
#include "value.h"
#include "function.h"
#include "printutils.h"
#include "memory.h"
#include <sstream>
#include <filesystem>
#include "io/fileutils.h"

namespace fs = std::filesystem;

#define YYMAXDEPTH 20000
#define LOC(loc) Location(loc.first_line, loc.first_column, loc.last_line, loc.last_column, sourcefile())
  
int parser_error_pos = -1;

int parserlex(void);
void yyerror(char const *s);

int lexerget_lineno(void);
std::shared_ptr<fs::path> sourcefile(void);
void lexer_set_parser_sourcefile(const fs::path& path);
int lexerlex_destroy(void);
int lexerlex(void);

std::stack<LocalScope *> scope_stack;
FileModule *rootmodule;

extern void lexerdestroy();
extern FILE *lexerin;
const char *parser_input_buffer;
static fs::path mainFilePath;
static std::string main_file_folder;

bool fileEnded=false;


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

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
/* Line 193 of yacc.c.  */
#line 227 "/Users/kintel/code/OpenSCAD/openscad/tests/parser.cxx"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

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


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 252 "/Users/kintel/code/OpenSCAD/openscad/tests/parser.cxx"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  38
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   699

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  53
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  29
/* YYNRULES -- Number of rules.  */
#define YYNRULES  103
/* YYNRULES -- Number of states.  */
#define YYNSTATES  212

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   286

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    34,     2,    51,     2,    39,     2,     2,
      48,    49,    37,    35,    52,    36,    42,    38,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    31,    45,
      32,    50,    33,    30,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    40,     2,    41,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    46,     2,    47,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    43,    44
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     5,     9,    12,    14,    18,    20,
      22,    23,    32,    33,    44,    46,    47,    50,    55,    58,
      61,    64,    67,    68,    72,    74,    76,    77,    82,    83,
      90,    91,    94,    97,    99,   103,   105,   107,   109,   111,
     113,   115,   117,   122,   124,   126,   128,   130,   134,   136,
     138,   144,   152,   156,   161,   165,   169,   173,   177,   181,
     185,   189,   193,   197,   201,   205,   209,   213,   216,   219,
     222,   226,   232,   237,   242,   248,   254,   260,   261,   263,
     269,   272,   278,   288,   294,   302,   304,   308,   310,   312,
     315,   316,   318,   320,   325,   326,   328,   333,   335,   339,
     340,   342,   347,   349
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      54,     0,    -1,    -1,    -1,    16,    55,    54,    -1,    56,
      54,    -1,    45,    -1,    46,    59,    47,    -1,    61,    -1,
      60,    -1,    -1,     5,    14,    48,    78,    76,    49,    57,
      56,    -1,    -1,     6,    14,    48,    78,    76,    49,    50,
      71,    58,    45,    -1,     4,    -1,    -1,    56,    59,    -1,
      14,    50,    71,    45,    -1,    34,    61,    -1,    51,    61,
      -1,    39,    61,    -1,    37,    61,    -1,    -1,    70,    62,
      68,    -1,    63,    -1,    65,    -1,    -1,    65,     8,    64,
      68,    -1,    -1,     7,    48,    71,    49,    66,    68,    -1,
      -1,    67,    68,    -1,    67,    60,    -1,    45,    -1,    46,
      67,    47,    -1,    61,    -1,    14,    -1,     9,    -1,    10,
      -1,    11,    -1,    12,    -1,    13,    -1,    69,    48,    80,
      49,    -1,    18,    -1,    19,    -1,    20,    -1,    14,    -1,
      71,    42,    14,    -1,    15,    -1,    17,    -1,    40,    71,
      31,    71,    41,    -1,    40,    71,    31,    71,    31,    71,
      41,    -1,    40,    76,    41,    -1,    40,    77,    76,    41,
      -1,    71,    37,    71,    -1,    71,    38,    71,    -1,    71,
      39,    71,    -1,    71,    35,    71,    -1,    71,    36,    71,
      -1,    71,    32,    71,    -1,    71,    21,    71,    -1,    71,
      23,    71,    -1,    71,    24,    71,    -1,    71,    22,    71,
      -1,    71,    33,    71,    -1,    71,    25,    71,    -1,    71,
      26,    71,    -1,    35,    71,    -1,    36,    71,    -1,    34,
      71,    -1,    48,    71,    49,    -1,    71,    30,    71,    31,
      71,    -1,    71,    40,    71,    41,    -1,    14,    48,    80,
      49,    -1,    10,    48,    80,    49,    71,    -1,    11,    48,
      80,    49,    72,    -1,    12,    48,    80,    49,    72,    -1,
      -1,    71,    -1,    10,    48,    80,    49,    74,    -1,    13,
      75,    -1,     9,    48,    80,    49,    75,    -1,     9,    48,
      80,    45,    71,    45,    80,    49,    75,    -1,     7,    48,
      71,    49,    75,    -1,     7,    48,    71,    49,    75,     8,
      75,    -1,    73,    -1,    48,    73,    49,    -1,    74,    -1,
      71,    -1,    52,    76,    -1,    -1,    71,    -1,    73,    -1,
      77,    52,    76,    75,    -1,    -1,    79,    -1,    78,    52,
      76,    79,    -1,    14,    -1,    14,    50,    71,    -1,    -1,
      81,    -1,    80,    52,    76,    81,    -1,    71,    -1,    14,
      50,    71,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   158,   158,   160,   159,   165,   169,   170,   171,   175,
     177,   176,   190,   189,   197,   203,   205,   209,   263,   268,
     273,   278,   284,   283,   293,   300,   305,   304,   317,   316,
     328,   330,   331,   335,   336,   337,   345,   346,   347,   348,
     349,   350,   354,   363,   367,   371,   375,   380,   385,   390,
     394,   398,   402,   406,   410,   414,   418,   422,   426,   430,
     434,   438,   442,   446,   450,   454,   458,   462,   466,   470,
     474,   478,   482,   486,   492,   497,   502,   510,   514,   523,
     528,   532,   545,   551,   555,   563,   564,   571,   572,   576,
     577,   581,   586,   591,   600,   603,   609,   618,   623,   632,
     635,   641,   650,   654
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_ERROR", "TOK_EOT", "TOK_MODULE",
  "TOK_FUNCTION", "TOK_IF", "TOK_ELSE", "TOK_FOR", "TOK_LET", "TOK_ASSERT",
  "TOK_ECHO", "TOK_EACH", "TOK_ID", "TOK_STRING", "TOK_USE", "TOK_NUMBER",
  "TOK_TRUE", "TOK_FALSE", "TOK_UNDEF", "LE", "GE", "EQ", "NE", "AND",
  "OR", "LET", "LOW_PRIO_RIGHT", "LOW_PRIO_LEFT", "'?'", "':'", "'<'",
  "'>'", "'!'", "'+'", "'-'", "'*'", "'/'", "'%'", "'['", "']'", "'.'",
  "HIGH_PRIO_RIGHT", "HIGH_PRIO_LEFT", "';'", "'{'", "'}'", "'('", "')'",
  "'='", "'#'", "','", "$accept", "input", "@1", "statement", "@2", "@3",
  "inner_input", "assignment", "module_instantiation", "@4",
  "ifelse_statement", "@5", "if_statement", "@6", "child_statements",
  "child_statement", "module_id", "single_module_instantiation", "expr",
  "expr_or_empty", "list_comprehension_elements",
  "list_comprehension_elements_p", "list_comprehension_elements_or_expr",
  "optional_commas", "vector_expr", "arguments_decl", "argument_decl",
  "arguments_call", "argument_call", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
      63,    58,    60,    62,    33,    43,    45,    42,    47,    37,
      91,    93,    46,   285,   286,    59,   123,   125,    40,    41,
      61,    35,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    53,    54,    55,    54,    54,    56,    56,    56,    56,
      57,    56,    58,    56,    56,    59,    59,    60,    61,    61,
      61,    61,    62,    61,    61,    63,    64,    63,    66,    65,
      67,    67,    67,    68,    68,    68,    69,    69,    69,    69,
      69,    69,    70,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    72,    72,    73,
      73,    73,    73,    73,    73,    74,    74,    75,    75,    76,
      76,    77,    77,    77,    78,    78,    78,    79,    79,    80,
      80,    80,    81,    81
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     0,     3,     2,     1,     3,     1,     1,
       0,     8,     0,    10,     1,     0,     2,     4,     2,     2,
       2,     2,     0,     3,     1,     1,     0,     4,     0,     6,
       0,     2,     2,     1,     3,     1,     1,     1,     1,     1,
       1,     1,     4,     1,     1,     1,     1,     3,     1,     1,
       5,     7,     3,     4,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       3,     5,     4,     4,     5,     5,     5,     0,     1,     5,
       2,     5,     9,     5,     7,     1,     3,     1,     1,     2,
       0,     1,     1,     4,     0,     1,     4,     1,     3,     0,
       1,     4,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,    14,     0,     0,     0,    37,    38,    39,    40,    41,
      36,     3,     0,     0,     0,     6,    15,     0,     0,     2,
       9,     8,    24,    25,     0,    22,     0,     0,     0,     0,
       2,    36,    18,    21,    20,    15,     0,    19,     1,     5,
      26,    99,     0,    94,    94,     0,     0,     0,    46,    48,
      49,    43,    44,    45,     0,     0,     0,    90,     0,     0,
       0,     4,    16,     7,     0,    46,   102,     0,   100,    33,
      30,    35,    23,    97,    90,    95,    90,    99,    99,    99,
      99,    69,    67,    68,     0,     0,     0,     0,    90,    91,
      92,     0,    90,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      28,    17,    27,     0,    42,    90,     0,     0,    90,     0,
       0,     0,     0,     0,     0,     0,    99,    99,     0,    88,
      85,    87,    80,    89,     0,    52,    90,     0,    70,    60,
      63,    61,    62,    65,    66,     0,    59,    64,    57,    58,
      54,    55,    56,     0,    47,     0,   103,     0,    34,    32,
      31,    98,    89,    10,     0,     0,    77,    77,    73,     0,
       0,     0,     0,     0,    89,    53,     0,    72,    29,   101,
      96,     0,     0,    74,    78,    75,    76,     0,     0,     0,
       0,    86,     0,    50,    93,    71,    11,    12,    83,     0,
      81,    79,     0,     0,     0,    99,    51,    13,    84,     0,
       0,    82
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    18,    30,    19,   181,   203,    36,    20,    21,    42,
      22,    64,    23,   155,   116,    72,    24,    25,    66,   185,
     130,   131,   132,    91,    92,    74,    75,    67,    68
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -168
static const yytype_int16 yypact[] =
{
     174,  -168,    -3,     3,   -24,  -168,  -168,  -168,  -168,  -168,
     -19,  -168,   305,   305,   305,  -168,   187,   305,    36,   174,
    -168,  -168,  -168,    30,    -7,  -168,     4,    10,   340,   340,
     174,  -168,  -168,  -168,  -168,   187,    18,  -168,  -168,  -168,
    -168,   367,   262,    26,    26,    16,    40,    42,    50,  -168,
    -168,  -168,  -168,  -168,   340,   340,   340,   230,   340,   387,
     453,  -168,  -168,  -168,   262,   -32,   613,     1,  -168,  -168,
    -168,  -168,  -168,    32,    49,  -168,    49,   367,   367,   367,
     367,   131,   131,   131,    54,    55,    56,   109,    73,   525,
    -168,    66,    78,   409,   340,   340,   340,   340,   340,   340,
     340,   340,   340,   340,   340,   340,   340,   340,   340,   117,
    -168,  -168,  -168,   340,  -168,    73,   246,   340,    73,    84,
      85,     5,    11,    34,    35,   340,   367,   367,   313,   613,
    -168,  -168,  -168,  -168,   340,  -168,    73,    95,  -168,     9,
       9,    75,    75,   657,   635,   547,     9,     9,   131,   131,
     -17,   -17,   -17,   569,  -168,   262,   613,   367,  -168,  -168,
    -168,   613,    26,  -168,    91,   340,   340,   340,  -168,   431,
     -10,    43,    93,   503,   109,  -168,   340,  -168,  -168,  -168,
    -168,   187,   340,   613,  -168,  -168,  -168,   109,   340,   109,
     109,  -168,   340,  -168,  -168,   613,  -168,   613,   142,   478,
    -168,  -168,   591,   106,   109,   367,  -168,  -168,  -168,    47,
     109,  -168
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -168,   -11,  -168,   -14,  -168,  -168,   120,    58,    -8,  -168,
    -168,  -168,  -168,  -168,  -168,   -61,  -168,  -168,   -28,   -15,
     -47,   -34,  -167,    17,  -168,   114,    13,   -65,     6
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      59,    60,    35,   112,    32,    33,    34,   194,    39,    37,
      90,    26,   121,   122,   123,   124,    80,    27,   113,    61,
     198,    35,   200,   108,    28,   109,    81,    82,    83,    89,
      93,    29,    96,    97,    71,   188,    38,   208,    40,   189,
      73,    41,   115,   211,   103,   104,   105,   106,   107,   108,
     114,   109,    43,   115,   165,   160,    71,   115,    44,   129,
     166,   170,   171,   115,    77,    63,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   172,   117,   167,   168,   156,   115,   115,    78,   161,
      79,   119,   190,   120,   178,   115,   210,   169,    80,   115,
      93,   118,   125,   126,   127,   133,   173,   135,    71,   137,
     103,   104,   105,   106,   107,   108,    84,   109,    85,    86,
      46,    47,    87,    48,    49,    88,    50,    51,    52,    53,
     136,   154,   157,   163,   164,   162,   175,   183,   184,   184,
     209,   182,   191,    54,    55,    56,   129,    71,   195,    57,
     204,   207,   186,   174,   197,    62,   201,   128,    76,   129,
     199,   129,   183,   179,   202,     0,     0,   196,   105,   106,
     107,   108,     0,   109,   159,   180,   129,     0,     1,     2,
       3,     4,   129,     5,     6,     7,     8,     9,    10,     0,
      11,     1,     2,     3,     4,     0,     5,     6,     7,     8,
       9,    10,     0,     0,     0,     0,     0,     0,    12,     0,
       0,    13,     0,    14,     0,     0,     0,     0,     0,    15,
      16,    12,     0,     0,    13,    17,    14,     0,     0,     0,
       0,     0,    15,    16,     0,     0,     0,    84,    17,    85,
      86,    46,    47,    87,    48,    49,     0,    50,    51,    52,
      53,     0,     0,     4,     0,     5,     6,     7,     8,     9,
      10,     0,     0,     0,    54,    55,    56,     0,     0,     4,
      57,     5,     6,     7,     8,     9,    31,     0,    58,     0,
      12,     0,    88,    13,     0,    14,     0,     0,     0,     0,
       0,    69,    70,   158,     0,     0,    12,    17,     0,    13,
       0,    14,     0,     0,     0,     0,     0,    69,    70,     0,
       0,     0,     4,    17,     5,     6,     7,     8,     9,    31,
      84,     0,    85,    86,    46,    47,    87,    48,    49,     0,
      50,    51,    52,    53,     0,     0,     0,     0,     0,    12,
       0,     0,    13,     0,    14,     0,     0,    54,    55,    56,
      45,    46,    47,    57,    48,    49,    17,    50,    51,    52,
      53,    58,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    54,    55,    56,    45,    46,    47,
      57,    65,    49,     0,    50,    51,    52,    53,    58,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    54,    55,    56,     0,     0,     0,    57,    94,    95,
      96,    97,    98,    99,     0,    58,     0,   100,     0,   101,
     102,     0,   103,   104,   105,   106,   107,   108,     0,   109,
      94,    95,    96,    97,    98,    99,   110,     0,     0,   100,
       0,   101,   102,     0,   103,   104,   105,   106,   107,   108,
       0,   109,    94,    95,    96,    97,    98,    99,   138,     0,
       0,   100,     0,   101,   102,     0,   103,   104,   105,   106,
     107,   108,     0,   109,    94,    95,    96,    97,    98,    99,
     187,     0,     0,   100,     0,   101,   102,     0,   103,   104,
     105,   106,   107,   108,     0,   109,     0,     0,   111,    94,
      95,    96,    97,    98,    99,     0,     0,     0,   100,     0,
     101,   102,     0,   103,   104,   105,   106,   107,   108,     0,
     109,     0,     0,   205,    94,    95,    96,    97,    98,    99,
       0,     0,     0,   100,   192,   101,   102,     0,   103,   104,
     105,   106,   107,   108,   193,   109,    94,    95,    96,    97,
      98,    99,     0,     0,     0,   100,   134,   101,   102,     0,
     103,   104,   105,   106,   107,   108,     0,   109,    94,    95,
      96,    97,    98,    99,     0,     0,     0,   100,   176,   101,
     102,     0,   103,   104,   105,   106,   107,   108,     0,   109,
      94,    95,    96,    97,    98,    99,     0,     0,     0,   100,
       0,   101,   102,     0,   103,   104,   105,   106,   107,   108,
     177,   109,    94,    95,    96,    97,    98,    99,     0,     0,
       0,   100,     0,   101,   102,     0,   103,   104,   105,   106,
     107,   108,   206,   109,    94,    95,    96,    97,    98,    99,
       0,     0,     0,   100,     0,   101,   102,     0,   103,   104,
     105,   106,   107,   108,     0,   109,    94,    95,    96,    97,
      98,     0,     0,     0,     0,     0,     0,   101,   102,     0,
     103,   104,   105,   106,   107,   108,     0,   109,    94,    95,
      96,    97,     0,     0,     0,     0,     0,     0,     0,   101,
     102,     0,   103,   104,   105,   106,   107,   108,     0,   109
};

static const yytype_int16 yycheck[] =
{
      28,    29,    16,    64,    12,    13,    14,   174,    19,    17,
      57,    14,    77,    78,    79,    80,    48,    14,    50,    30,
     187,    35,   189,    40,    48,    42,    54,    55,    56,    57,
      58,    50,    23,    24,    42,    45,     0,   204,     8,    49,
      14,    48,    52,   210,    35,    36,    37,    38,    39,    40,
      49,    42,    48,    52,    49,   116,    64,    52,    48,    87,
      49,   126,   127,    52,    48,    47,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   128,    50,    49,    49,   113,    52,    52,    48,   117,
      48,    74,    49,    76,   155,    52,    49,   125,    48,    52,
     128,    52,    48,    48,    48,    88,   134,    41,   116,    92,
      35,    36,    37,    38,    39,    40,     7,    42,     9,    10,
      11,    12,    13,    14,    15,    52,    17,    18,    19,    20,
      52,    14,   115,    49,    49,   118,    41,   165,   166,   167,
     205,    50,    49,    34,    35,    36,   174,   155,   176,    40,
       8,    45,   167,   136,   182,    35,   190,    48,    44,   187,
     188,   189,   190,   157,   192,    -1,    -1,   181,    37,    38,
      39,    40,    -1,    42,   116,   162,   204,    -1,     4,     5,
       6,     7,   210,     9,    10,    11,    12,    13,    14,    -1,
      16,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    14,    -1,    -1,    -1,    -1,    -1,    -1,    34,    -1,
      -1,    37,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      46,    34,    -1,    -1,    37,    51,    39,    -1,    -1,    -1,
      -1,    -1,    45,    46,    -1,    -1,    -1,     7,    51,     9,
      10,    11,    12,    13,    14,    15,    -1,    17,    18,    19,
      20,    -1,    -1,     7,    -1,     9,    10,    11,    12,    13,
      14,    -1,    -1,    -1,    34,    35,    36,    -1,    -1,     7,
      40,     9,    10,    11,    12,    13,    14,    -1,    48,    -1,
      34,    -1,    52,    37,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    46,    47,    -1,    -1,    34,    51,    -1,    37,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    46,    -1,
      -1,    -1,     7,    51,     9,    10,    11,    12,    13,    14,
       7,    -1,     9,    10,    11,    12,    13,    14,    15,    -1,
      17,    18,    19,    20,    -1,    -1,    -1,    -1,    -1,    34,
      -1,    -1,    37,    -1,    39,    -1,    -1,    34,    35,    36,
      10,    11,    12,    40,    14,    15,    51,    17,    18,    19,
      20,    48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    34,    35,    36,    10,    11,    12,
      40,    14,    15,    -1,    17,    18,    19,    20,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    35,    36,    -1,    -1,    -1,    40,    21,    22,
      23,    24,    25,    26,    -1,    48,    -1,    30,    -1,    32,
      33,    -1,    35,    36,    37,    38,    39,    40,    -1,    42,
      21,    22,    23,    24,    25,    26,    49,    -1,    -1,    30,
      -1,    32,    33,    -1,    35,    36,    37,    38,    39,    40,
      -1,    42,    21,    22,    23,    24,    25,    26,    49,    -1,
      -1,    30,    -1,    32,    33,    -1,    35,    36,    37,    38,
      39,    40,    -1,    42,    21,    22,    23,    24,    25,    26,
      49,    -1,    -1,    30,    -1,    32,    33,    -1,    35,    36,
      37,    38,    39,    40,    -1,    42,    -1,    -1,    45,    21,
      22,    23,    24,    25,    26,    -1,    -1,    -1,    30,    -1,
      32,    33,    -1,    35,    36,    37,    38,    39,    40,    -1,
      42,    -1,    -1,    45,    21,    22,    23,    24,    25,    26,
      -1,    -1,    -1,    30,    31,    32,    33,    -1,    35,    36,
      37,    38,    39,    40,    41,    42,    21,    22,    23,    24,
      25,    26,    -1,    -1,    -1,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    39,    40,    -1,    42,    21,    22,
      23,    24,    25,    26,    -1,    -1,    -1,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    39,    40,    -1,    42,
      21,    22,    23,    24,    25,    26,    -1,    -1,    -1,    30,
      -1,    32,    33,    -1,    35,    36,    37,    38,    39,    40,
      41,    42,    21,    22,    23,    24,    25,    26,    -1,    -1,
      -1,    30,    -1,    32,    33,    -1,    35,    36,    37,    38,
      39,    40,    41,    42,    21,    22,    23,    24,    25,    26,
      -1,    -1,    -1,    30,    -1,    32,    33,    -1,    35,    36,
      37,    38,    39,    40,    -1,    42,    21,    22,    23,    24,
      25,    -1,    -1,    -1,    -1,    -1,    -1,    32,    33,    -1,
      35,    36,    37,    38,    39,    40,    -1,    42,    21,    22,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      33,    -1,    35,    36,    37,    38,    39,    40,    -1,    42
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     4,     5,     6,     7,     9,    10,    11,    12,    13,
      14,    16,    34,    37,    39,    45,    46,    51,    54,    56,
      60,    61,    63,    65,    69,    70,    14,    14,    48,    50,
      55,    14,    61,    61,    61,    56,    59,    61,     0,    54,
       8,    48,    62,    48,    48,    10,    11,    12,    14,    15,
      17,    18,    19,    20,    34,    35,    36,    40,    48,    71,
      71,    54,    59,    47,    64,    14,    71,    80,    81,    45,
      46,    61,    68,    14,    78,    79,    78,    48,    48,    48,
      48,    71,    71,    71,     7,     9,    10,    13,    52,    71,
      73,    76,    77,    71,    21,    22,    23,    24,    25,    26,
      30,    32,    33,    35,    36,    37,    38,    39,    40,    42,
      49,    45,    68,    50,    49,    52,    67,    50,    52,    76,
      76,    80,    80,    80,    80,    48,    48,    48,    48,    71,
      73,    74,    75,    76,    31,    41,    52,    76,    49,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    14,    66,    71,    76,    47,    60,
      68,    71,    76,    49,    49,    49,    49,    49,    49,    71,
      80,    80,    73,    71,    76,    41,    31,    41,    68,    81,
      79,    57,    50,    71,    71,    72,    72,    49,    45,    49,
      49,    49,    31,    41,    75,    71,    56,    71,    75,    71,
      75,    74,    71,    58,     8,    45,    41,    45,    75,    80,
      49,    75
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 160 "../src/parser.y"
    {
              rootmodule->registerUse(std::string((yyvsp[(1) - (1)].text)));
              free((yyvsp[(1) - (1)].text));
            ;}
    break;

  case 8:
#line 172 "../src/parser.y"
    {
              if ((yyvsp[(1) - (1)].inst)) scope_stack.top()->addChild((yyvsp[(1) - (1)].inst));
            ;}
    break;

  case 10:
#line 177 "../src/parser.y"
    {
              UserModule *newmodule = new UserModule((yyvsp[(2) - (6)].text), LOC((yyloc)));
              newmodule->definition_arguments = *(yyvsp[(4) - (6)].args);
              scope_stack.top()->addModule((yyvsp[(2) - (6)].text), newmodule);
              scope_stack.push(&newmodule->scope);
              free((yyvsp[(2) - (6)].text));
              delete (yyvsp[(4) - (6)].args);
            ;}
    break;

  case 11:
#line 186 "../src/parser.y"
    {
                scope_stack.pop();
            ;}
    break;

  case 12:
#line 190 "../src/parser.y"
    {
              UserFunction *func = UserFunction::create((yyvsp[(2) - (8)].text), *(yyvsp[(4) - (8)].args), shared_ptr<Expression>((yyvsp[(8) - (8)].expr)), LOC((yyloc)));
              scope_stack.top()->addFunction(func);
              free((yyvsp[(2) - (8)].text));
              delete (yyvsp[(4) - (8)].args);
            ;}
    break;

  case 14:
#line 198 "../src/parser.y"
    {
                fileEnded=true;
            ;}
    break;

  case 17:
#line 210 "../src/parser.y"
    {
                bool found = false;
                for (auto &assignment : scope_stack.top()->assignments) {
                    if (assignment.name == (yyvsp[(1) - (4)].text)) {
                        auto mainFile = mainFilePath.string();
                        auto prevFile = assignment.location().fileName();
                        auto currFile = LOC((yyloc)).fileName();
                        
                        const auto uncPathCurr = fs_uncomplete(currFile, mainFilePath.parent_path());
                        const auto uncPathPrev = fs_uncomplete(prevFile, mainFilePath.parent_path());

                        if(fileEnded){
                            //assigments via commandline
                        }else if(prevFile==mainFile && currFile == mainFile){
                            //both assigments in the mainFile
                            PRINTB("WARNING: %s was assigned on line %i but was overwritten on line %i",
                                    assignment.name%
                                    assignment.location().firstLine()%
                                    LOC((yyloc)).firstLine());
                        }else if(uncPathCurr == uncPathPrev){
                            //assigment overwritten within the same file
                            //the line number beeing equal happens, when a file is included multiple times
                            if(assignment.location().firstLine() != LOC((yyloc)).firstLine()){
                                PRINTB("WARNING: %s was assigned on line %i of %s but was overwritten on line %i",
                                        assignment.name%
                                        assignment.location().firstLine()%
                                        uncPathPrev%
                                        LOC((yyloc)).firstLine());
                            }
                        }else if(prevFile==mainFile && currFile != mainFile){
                            //assigment from the mainFile overwritten by an include
                            PRINTB("WARNING: %s was assigned on line %i of %s but was overwritten on line %i of %s",
                                    assignment.name%
                                    assignment.location().firstLine()%
                                    uncPathPrev%
                                    LOC((yyloc)).firstLine()%
                                    uncPathCurr);
                        }

                        assignment.expr = shared_ptr<Expression>((yyvsp[(3) - (4)].expr));
                        assignment.setLocation(LOC((yyloc)));
                        found = true;
                        break;
                    }
                }
                if (!found) {
                  scope_stack.top()->addAssignment(Assignment((yyvsp[(1) - (4)].text), shared_ptr<Expression>((yyvsp[(3) - (4)].expr)), LOC((yyloc))));
                }
                free((yyvsp[(1) - (4)].text));
            ;}
    break;

  case 18:
#line 264 "../src/parser.y"
    {
                (yyval.inst) = (yyvsp[(2) - (2)].inst);
                if ((yyval.inst)) (yyval.inst)->tag_root = true;
            ;}
    break;

  case 19:
#line 269 "../src/parser.y"
    {
                (yyval.inst) = (yyvsp[(2) - (2)].inst);
                if ((yyval.inst)) (yyval.inst)->tag_highlight = true;
            ;}
    break;

  case 20:
#line 274 "../src/parser.y"
    {
                (yyval.inst) = (yyvsp[(2) - (2)].inst);
                if ((yyval.inst)) (yyval.inst)->tag_background = true;
            ;}
    break;

  case 21:
#line 279 "../src/parser.y"
    {
                delete (yyvsp[(2) - (2)].inst);
                (yyval.inst) = NULL;
            ;}
    break;

  case 22:
#line 284 "../src/parser.y"
    {
                (yyval.inst) = (yyvsp[(1) - (1)].inst);
                scope_stack.push(&(yyvsp[(1) - (1)].inst)->scope);
            ;}
    break;

  case 23:
#line 289 "../src/parser.y"
    {
                scope_stack.pop();
                (yyval.inst) = (yyvsp[(2) - (3)].inst);
            ;}
    break;

  case 24:
#line 294 "../src/parser.y"
    {
                (yyval.inst) = (yyvsp[(1) - (1)].ifelse);
            ;}
    break;

  case 25:
#line 301 "../src/parser.y"
    {
                (yyval.ifelse) = (yyvsp[(1) - (1)].ifelse);
            ;}
    break;

  case 26:
#line 305 "../src/parser.y"
    {
                scope_stack.push(&(yyvsp[(1) - (2)].ifelse)->else_scope);
            ;}
    break;

  case 27:
#line 309 "../src/parser.y"
    {
                scope_stack.pop();
                (yyval.ifelse) = (yyvsp[(1) - (4)].ifelse);
            ;}
    break;

  case 28:
#line 317 "../src/parser.y"
    {
                (yyval.ifelse) = new IfElseModuleInstantiation(shared_ptr<Expression>((yyvsp[(3) - (4)].expr)), main_file_folder, LOC((yyloc)));
                scope_stack.push(&(yyval.ifelse)->scope);
            ;}
    break;

  case 29:
#line 322 "../src/parser.y"
    {
                scope_stack.pop();
                (yyval.ifelse) = (yyvsp[(5) - (6)].ifelse);
            ;}
    break;

  case 35:
#line 338 "../src/parser.y"
    {
                if ((yyvsp[(1) - (1)].inst)) scope_stack.top()->addChild((yyvsp[(1) - (1)].inst));
            ;}
    break;

  case 36:
#line 345 "../src/parser.y"
    { (yyval.text) = (yyvsp[(1) - (1)].text); ;}
    break;

  case 37:
#line 346 "../src/parser.y"
    { (yyval.text) = strdup("for"); ;}
    break;

  case 38:
#line 347 "../src/parser.y"
    { (yyval.text) = strdup("let"); ;}
    break;

  case 39:
#line 348 "../src/parser.y"
    { (yyval.text) = strdup("assert"); ;}
    break;

  case 40:
#line 349 "../src/parser.y"
    { (yyval.text) = strdup("echo"); ;}
    break;

  case 41:
#line 350 "../src/parser.y"
    { (yyval.text) = strdup("each"); ;}
    break;

  case 42:
#line 355 "../src/parser.y"
    {
                (yyval.inst) = new ModuleInstantiation((yyvsp[(1) - (4)].text), *(yyvsp[(3) - (4)].args), main_file_folder, LOC((yyloc)));
                free((yyvsp[(1) - (4)].text));
                delete (yyvsp[(3) - (4)].args);
            ;}
    break;

  case 43:
#line 364 "../src/parser.y"
    {
              (yyval.expr) = new Literal(ValuePtr(true), LOC((yyloc)));
            ;}
    break;

  case 44:
#line 368 "../src/parser.y"
    {
              (yyval.expr) = new Literal(ValuePtr(false), LOC((yyloc)));
            ;}
    break;

  case 45:
#line 372 "../src/parser.y"
    {
              (yyval.expr) = new Literal(ValuePtr::undefined, LOC((yyloc)));
            ;}
    break;

  case 46:
#line 376 "../src/parser.y"
    {
              (yyval.expr) = new Lookup((yyvsp[(1) - (1)].text), LOC((yyloc)));
                free((yyvsp[(1) - (1)].text));
            ;}
    break;

  case 47:
#line 381 "../src/parser.y"
    {
              (yyval.expr) = new MemberLookup((yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].text), LOC((yyloc)));
              free((yyvsp[(3) - (3)].text));
            ;}
    break;

  case 48:
#line 386 "../src/parser.y"
    {
              (yyval.expr) = new Literal(ValuePtr(std::string((yyvsp[(1) - (1)].text))), LOC((yyloc)));
              free((yyvsp[(1) - (1)].text));
            ;}
    break;

  case 49:
#line 391 "../src/parser.y"
    {
              (yyval.expr) = new Literal(ValuePtr((yyvsp[(1) - (1)].number)), LOC((yyloc)));
            ;}
    break;

  case 50:
#line 395 "../src/parser.y"
    {
              (yyval.expr) = new Range((yyvsp[(2) - (5)].expr), (yyvsp[(4) - (5)].expr), LOC((yyloc)));
            ;}
    break;

  case 51:
#line 399 "../src/parser.y"
    {
              (yyval.expr) = new Range((yyvsp[(2) - (7)].expr), (yyvsp[(4) - (7)].expr), (yyvsp[(6) - (7)].expr), LOC((yyloc)));
            ;}
    break;

  case 52:
#line 403 "../src/parser.y"
    {
              (yyval.expr) = new Literal(ValuePtr(Value::VectorType()), LOC((yyloc)));
            ;}
    break;

  case 53:
#line 407 "../src/parser.y"
    {
              (yyval.expr) = (yyvsp[(2) - (4)].vec);
            ;}
    break;

  case 54:
#line 411 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::Multiply, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 55:
#line 415 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::Divide, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 56:
#line 419 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::Modulo, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 57:
#line 423 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::Plus, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 58:
#line 427 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::Minus, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 59:
#line 431 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::Less, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 60:
#line 435 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::LessEqual, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 61:
#line 439 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::Equal, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 62:
#line 443 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::NotEqual, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 63:
#line 447 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::GreaterEqual, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 64:
#line 451 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::Greater, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 65:
#line 455 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::LogicalAnd, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 66:
#line 459 "../src/parser.y"
    {
              (yyval.expr) = new BinaryOp((yyvsp[(1) - (3)].expr), BinaryOp::Op::LogicalOr, (yyvsp[(3) - (3)].expr), LOC((yyloc)));
            ;}
    break;

  case 67:
#line 463 "../src/parser.y"
    {
                (yyval.expr) = (yyvsp[(2) - (2)].expr);
            ;}
    break;

  case 68:
#line 467 "../src/parser.y"
    {
              (yyval.expr) = new UnaryOp(UnaryOp::Op::Negate, (yyvsp[(2) - (2)].expr), LOC((yyloc)));
            ;}
    break;

  case 69:
#line 471 "../src/parser.y"
    {
              (yyval.expr) = new UnaryOp(UnaryOp::Op::Not, (yyvsp[(2) - (2)].expr), LOC((yyloc)));
            ;}
    break;

  case 70:
#line 475 "../src/parser.y"
    {
              (yyval.expr) = (yyvsp[(2) - (3)].expr);
            ;}
    break;

  case 71:
#line 479 "../src/parser.y"
    {
              (yyval.expr) = new TernaryOp((yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr), LOC((yyloc)));
            ;}
    break;

  case 72:
#line 483 "../src/parser.y"
    {
              (yyval.expr) = new ArrayLookup((yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr), LOC((yyloc)));
            ;}
    break;

  case 73:
#line 487 "../src/parser.y"
    {
              (yyval.expr) = new FunctionCall((yyvsp[(1) - (4)].text), *(yyvsp[(3) - (4)].args), LOC((yyloc)));
              free((yyvsp[(1) - (4)].text));
              delete (yyvsp[(3) - (4)].args);
            ;}
    break;

  case 74:
#line 493 "../src/parser.y"
    {
              (yyval.expr) = FunctionCall::create("let", *(yyvsp[(3) - (5)].args), (yyvsp[(5) - (5)].expr), LOC((yyloc)));
              delete (yyvsp[(3) - (5)].args);
            ;}
    break;

  case 75:
#line 498 "../src/parser.y"
    {
              (yyval.expr) = FunctionCall::create("assert", *(yyvsp[(3) - (5)].args), (yyvsp[(5) - (5)].expr), LOC((yyloc)));
              delete (yyvsp[(3) - (5)].args);
            ;}
    break;

  case 76:
#line 503 "../src/parser.y"
    {
              (yyval.expr) = FunctionCall::create("echo", *(yyvsp[(3) - (5)].args), (yyvsp[(5) - (5)].expr), LOC((yyloc)));
              delete (yyvsp[(3) - (5)].args);
            ;}
    break;

  case 77:
#line 511 "../src/parser.y"
    {
              (yyval.expr) = NULL;
            ;}
    break;

  case 78:
#line 515 "../src/parser.y"
    {
              (yyval.expr) = (yyvsp[(1) - (1)].expr);
            ;}
    break;

  case 79:
#line 524 "../src/parser.y"
    {
              (yyval.expr) = new LcLet(*(yyvsp[(3) - (5)].args), (yyvsp[(5) - (5)].expr), LOC((yyloc)));
              delete (yyvsp[(3) - (5)].args);
            ;}
    break;

  case 80:
#line 529 "../src/parser.y"
    {
              (yyval.expr) = new LcEach((yyvsp[(2) - (2)].expr), LOC((yyloc)));
            ;}
    break;

  case 81:
#line 533 "../src/parser.y"
    {
                (yyval.expr) = (yyvsp[(5) - (5)].expr);

                /* transform for(i=...,j=...) -> for(i=...) for(j=...) */
                for (int i = (yyvsp[(3) - (5)].args)->size()-1; i >= 0; i--) {
                  AssignmentList arglist;
                  arglist.push_back((*(yyvsp[(3) - (5)].args))[i]);
                  Expression *e = new LcFor(arglist, (yyval.expr), LOC((yyloc)));
                    (yyval.expr) = e;
                }
                delete (yyvsp[(3) - (5)].args);
            ;}
    break;

  case 82:
#line 546 "../src/parser.y"
    {
              (yyval.expr) = new LcForC(*(yyvsp[(3) - (9)].args), *(yyvsp[(7) - (9)].args), (yyvsp[(5) - (9)].expr), (yyvsp[(9) - (9)].expr), LOC((yyloc)));
                delete (yyvsp[(3) - (9)].args);
                delete (yyvsp[(7) - (9)].args);
            ;}
    break;

  case 83:
#line 552 "../src/parser.y"
    {
              (yyval.expr) = new LcIf((yyvsp[(3) - (5)].expr), (yyvsp[(5) - (5)].expr), 0, LOC((yyloc)));
            ;}
    break;

  case 84:
#line 556 "../src/parser.y"
    {
              (yyval.expr) = new LcIf((yyvsp[(3) - (7)].expr), (yyvsp[(5) - (7)].expr), (yyvsp[(7) - (7)].expr), LOC((yyloc)));
            ;}
    break;

  case 86:
#line 565 "../src/parser.y"
    {
                (yyval.expr) = (yyvsp[(2) - (3)].expr);
            ;}
    break;

  case 91:
#line 582 "../src/parser.y"
    {
              (yyval.vec) = new Vector(LOC((yyloc)));
              (yyval.vec)->push_back((yyvsp[(1) - (1)].expr));
            ;}
    break;

  case 92:
#line 587 "../src/parser.y"
    {
              (yyval.vec) = new Vector(LOC((yyloc)));
              (yyval.vec)->push_back((yyvsp[(1) - (1)].expr));
            ;}
    break;

  case 93:
#line 592 "../src/parser.y"
    {
              (yyval.vec) = (yyvsp[(1) - (4)].vec);
              (yyval.vec)->push_back((yyvsp[(4) - (4)].expr));
            ;}
    break;

  case 94:
#line 600 "../src/parser.y"
    {
                (yyval.args) = new AssignmentList();
            ;}
    break;

  case 95:
#line 604 "../src/parser.y"
    {
                (yyval.args) = new AssignmentList();
                (yyval.args)->push_back(*(yyvsp[(1) - (1)].arg));
                delete (yyvsp[(1) - (1)].arg);
            ;}
    break;

  case 96:
#line 610 "../src/parser.y"
    {
                (yyval.args) = (yyvsp[(1) - (4)].args);
                (yyval.args)->push_back(*(yyvsp[(4) - (4)].arg));
                delete (yyvsp[(4) - (4)].arg);
            ;}
    break;

  case 97:
#line 619 "../src/parser.y"
    {
                (yyval.arg) = new Assignment((yyvsp[(1) - (1)].text), LOC((yyloc)));
                free((yyvsp[(1) - (1)].text));
            ;}
    break;

  case 98:
#line 624 "../src/parser.y"
    {
              (yyval.arg) = new Assignment((yyvsp[(1) - (3)].text), shared_ptr<Expression>((yyvsp[(3) - (3)].expr)), LOC((yyloc)));
                free((yyvsp[(1) - (3)].text));
            ;}
    break;

  case 99:
#line 632 "../src/parser.y"
    {
                (yyval.args) = new AssignmentList();
            ;}
    break;

  case 100:
#line 636 "../src/parser.y"
    {
                (yyval.args) = new AssignmentList();
                (yyval.args)->push_back(*(yyvsp[(1) - (1)].arg));
                delete (yyvsp[(1) - (1)].arg);
            ;}
    break;

  case 101:
#line 642 "../src/parser.y"
    {
                (yyval.args) = (yyvsp[(1) - (4)].args);
                (yyval.args)->push_back(*(yyvsp[(4) - (4)].arg));
                delete (yyvsp[(4) - (4)].arg);
            ;}
    break;

  case 102:
#line 651 "../src/parser.y"
    {
                (yyval.arg) = new Assignment("", shared_ptr<Expression>((yyvsp[(1) - (1)].expr)), LOC((yyloc)));
            ;}
    break;

  case 103:
#line 655 "../src/parser.y"
    {
                (yyval.arg) = new Assignment((yyvsp[(1) - (3)].text), shared_ptr<Expression>((yyvsp[(3) - (3)].expr)), LOC((yyloc)));
                free((yyvsp[(1) - (3)].text));
            ;}
    break;


/* Line 1267 of yacc.c.  */
#line 2426 "/Users/kintel/code/OpenSCAD/openscad/tests/parser.cxx"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 661 "../src/parser.y"


int parserlex(void)
{
  return lexerlex();
}

void yyerror (char const *s)
{
  // FIXME: We leak memory on parser errors...
  PRINTB("ERROR: Parser error in file %s, line %d: %s\n",
         (*sourcefile()) % lexerget_lineno() % s);
}

bool parse(FileModule *&module, const std::string& text, const std::string &filename, const std::string &mainFile, int debug)
{
  fs::path parser_sourcefile = fs::absolute(fs::path(filename));
  main_file_folder = parser_sourcefile.parent_path().generic_string();
  lexer_set_parser_sourcefile(parser_sourcefile);
  mainFilePath = mainFile;

  lexerin = NULL;
  parser_error_pos = -1;
  parser_input_buffer = text.c_str();
  fileEnded=false;

  rootmodule = new FileModule(main_file_folder, parser_sourcefile.filename().generic_string());
  scope_stack.push(&rootmodule->scope);
  //        PRINTB_NOCACHE("New module: %s %p", "root" % rootmodule);

  parserdebug = debug;
  int parserretval = parserparse();

  lexerdestroy();
  lexerlex_destroy();

  module = rootmodule;
  if (parserretval != 0) return false;

  parser_error_pos = -1;
  parser_input_buffer = nullptr;
  scope_stack.pop();

  return true;
}


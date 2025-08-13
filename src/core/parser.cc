/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the variable and function names.  */
#define yyparse parserparse
#define yylex parserlex
#define yyerror parsererror
#define yydebug parserdebug
#define yynerrs parsernerrs
#define yylval parserlval
#define yychar parserchar
#define yylloc parserlloc

/* First part of user prologue.  */
#line 29 "src/core/parser.y"

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

bool fileEnded = false;

#line 138 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"

#ifndef YY_CAST
#ifdef __cplusplus
#define YY_CAST(Type, Val) static_cast<Type>(Val)
#define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type>(Val)
#else
#define YY_CAST(Type, Val) ((Type)(Val))
#define YY_REINTERPRET_CAST(Type, Val) ((Type)(Val))
#endif
#endif
#ifndef YY_NULLPTR
#if defined __cplusplus
#if 201103L <= __cplusplus
#define YY_NULLPTR nullptr
#else
#define YY_NULLPTR 0
#endif
#else
#define YY_NULLPTR ((void *)0)
#endif
#endif

#include "parser.hxx"
/* Symbol kind.  */
enum yysymbol_kind_t {
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                          /* "end of file"  */
  YYSYMBOL_YYerror = 1,                        /* error  */
  YYSYMBOL_YYUNDEF = 2,                        /* "invalid token"  */
  YYSYMBOL_TOK_ERROR = 3,                      /* TOK_ERROR  */
  YYSYMBOL_TOK_EOT = 4,                        /* TOK_EOT  */
  YYSYMBOL_TOK_MODULE = 5,                     /* TOK_MODULE  */
  YYSYMBOL_TOK_FUNCTION = 6,                   /* TOK_FUNCTION  */
  YYSYMBOL_TOK_IF = 7,                         /* TOK_IF  */
  YYSYMBOL_TOK_ELSE = 8,                       /* TOK_ELSE  */
  YYSYMBOL_TOK_FOR = 9,                        /* TOK_FOR  */
  YYSYMBOL_TOK_LET = 10,                       /* TOK_LET  */
  YYSYMBOL_TOK_ASSERT = 11,                    /* TOK_ASSERT  */
  YYSYMBOL_TOK_ECHO = 12,                      /* TOK_ECHO  */
  YYSYMBOL_TOK_EACH = 13,                      /* TOK_EACH  */
  YYSYMBOL_TOK_ID = 14,                        /* TOK_ID  */
  YYSYMBOL_TOK_STRING = 15,                    /* TOK_STRING  */
  YYSYMBOL_TOK_USE = 16,                       /* TOK_USE  */
  YYSYMBOL_TOK_NUMBER = 17,                    /* TOK_NUMBER  */
  YYSYMBOL_TOK_TRUE = 18,                      /* TOK_TRUE  */
  YYSYMBOL_TOK_FALSE = 19,                     /* TOK_FALSE  */
  YYSYMBOL_TOK_UNDEF = 20,                     /* TOK_UNDEF  */
  YYSYMBOL_LE = 21,                            /* LE  */
  YYSYMBOL_GE = 22,                            /* GE  */
  YYSYMBOL_EQ = 23,                            /* EQ  */
  YYSYMBOL_NEQ = 24,                           /* NEQ  */
  YYSYMBOL_AND = 25,                           /* AND  */
  YYSYMBOL_OR = 26,                            /* OR  */
  YYSYMBOL_NO_ELSE = 27,                       /* NO_ELSE  */
  YYSYMBOL_28_ = 28,                           /* ';'  */
  YYSYMBOL_29_ = 29,                           /* '{'  */
  YYSYMBOL_30_ = 30,                           /* '}'  */
  YYSYMBOL_31_ = 31,                           /* '('  */
  YYSYMBOL_32_ = 32,                           /* ')'  */
  YYSYMBOL_33_ = 33,                           /* '='  */
  YYSYMBOL_34_ = 34,                           /* '!'  */
  YYSYMBOL_35_ = 35,                           /* '#'  */
  YYSYMBOL_36_ = 36,                           /* '%'  */
  YYSYMBOL_37_ = 37,                           /* '*'  */
  YYSYMBOL_38_ = 38,                           /* '.'  */
  YYSYMBOL_39_ = 39,                           /* '?'  */
  YYSYMBOL_40_ = 40,                           /* ':'  */
  YYSYMBOL_41_ = 41,                           /* '>'  */
  YYSYMBOL_42_ = 42,                           /* '<'  */
  YYSYMBOL_43_ = 43,                           /* '+'  */
  YYSYMBOL_44_ = 44,                           /* '-'  */
  YYSYMBOL_45_ = 45,                           /* '/'  */
  YYSYMBOL_46_ = 46,                           /* '^'  */
  YYSYMBOL_47_ = 47,                           /* '['  */
  YYSYMBOL_48_ = 48,                           /* ']'  */
  YYSYMBOL_49_ = 49,                           /* ','  */
  YYSYMBOL_YYACCEPT = 50,                      /* $accept  */
  YYSYMBOL_input = 51,                         /* input  */
  YYSYMBOL_statement = 52,                     /* statement  */
  YYSYMBOL_53_1 = 53,                          /* $@1  */
  YYSYMBOL_inner_input = 54,                   /* inner_input  */
  YYSYMBOL_assignment = 55,                    /* assignment  */
  YYSYMBOL_module_instantiation = 56,          /* module_instantiation  */
  YYSYMBOL_57_2 = 57,                          /* @2  */
  YYSYMBOL_ifelse_statement = 58,              /* ifelse_statement  */
  YYSYMBOL_59_3 = 59,                          /* $@3  */
  YYSYMBOL_if_statement = 60,                  /* if_statement  */
  YYSYMBOL_61_4 = 61,                          /* @4  */
  YYSYMBOL_child_statements = 62,              /* child_statements  */
  YYSYMBOL_child_statement = 63,               /* child_statement  */
  YYSYMBOL_module_id = 64,                     /* module_id  */
  YYSYMBOL_single_module_instantiation = 65,   /* single_module_instantiation  */
  YYSYMBOL_expr = 66,                          /* expr  */
  YYSYMBOL_logic_or = 67,                      /* logic_or  */
  YYSYMBOL_logic_and = 68,                     /* logic_and  */
  YYSYMBOL_equality = 69,                      /* equality  */
  YYSYMBOL_comparison = 70,                    /* comparison  */
  YYSYMBOL_addition = 71,                      /* addition  */
  YYSYMBOL_multiplication = 72,                /* multiplication  */
  YYSYMBOL_unary = 73,                         /* unary  */
  YYSYMBOL_exponent = 74,                      /* exponent  */
  YYSYMBOL_call = 75,                          /* call  */
  YYSYMBOL_primary = 76,                       /* primary  */
  YYSYMBOL_expr_or_empty = 77,                 /* expr_or_empty  */
  YYSYMBOL_list_comprehension_elements = 78,   /* list_comprehension_elements  */
  YYSYMBOL_list_comprehension_elements_p = 79, /* list_comprehension_elements_p  */
  YYSYMBOL_optional_trailing_comma = 80,       /* optional_trailing_comma  */
  YYSYMBOL_vector_elements = 81,               /* vector_elements  */
  YYSYMBOL_vector_element = 82,                /* vector_element  */
  YYSYMBOL_parameters = 83,                    /* parameters  */
  YYSYMBOL_parameter_list = 84,                /* parameter_list  */
  YYSYMBOL_parameter = 85,                     /* parameter  */
  YYSYMBOL_arguments = 86,                     /* arguments  */
  YYSYMBOL_argument_list = 87,                 /* argument_list  */
  YYSYMBOL_argument = 88                       /* argument  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;

#ifdef short
#undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
#include <limits.h> /* INFRINGES ON USER NAME SPACE */
#if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#define YY_STDINT_H
#endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
#undef UINT_LEAST8_MAX
#undef UINT_LEAST16_MAX
#define UINT_LEAST8_MAX 255
#define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
#if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#define YYPTRDIFF_T __PTRDIFF_TYPE__
#define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
#elif defined PTRDIFF_MAX
#ifndef ptrdiff_t
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#endif
#define YYPTRDIFF_T ptrdiff_t
#define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
#else
#define YYPTRDIFF_T long
#define YYPTRDIFF_MAXIMUM LONG_MAX
#endif
#endif

#ifndef YYSIZE_T
#ifdef __SIZE_TYPE__
#define YYSIZE_T __SIZE_TYPE__
#elif defined size_t
#define YYSIZE_T size_t
#elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#define YYSIZE_T size_t
#else
#define YYSIZE_T unsigned
#endif
#endif

#define YYSIZE_MAXIMUM \
  YY_CAST(YYPTRDIFF_T, \
          (YYPTRDIFF_MAXIMUM < YY_CAST(YYSIZE_T, -1) ? YYPTRDIFF_MAXIMUM : YY_CAST(YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST(YYPTRDIFF_T, sizeof(X))

/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
#if defined YYENABLE_NLS && YYENABLE_NLS
#if ENABLE_NLS
#include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#define YY_(Msgid) dgettext("bison-runtime", Msgid)
#endif
#endif
#ifndef YY_
#define YY_(Msgid) Msgid
#endif
#endif

#ifndef YY_ATTRIBUTE_PURE
#if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#define YY_ATTRIBUTE_PURE __attribute__((__pure__))
#else
#define YY_ATTRIBUTE_PURE
#endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
#if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#define YY_ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#define YY_ATTRIBUTE_UNUSED
#endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if !defined lint || defined __GNUC__
#define YY_USE(E) ((void)(E))
#else
#define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && !defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
#if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")
#else
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                                            \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuninitialized\"") \
    _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#endif
#define YY_IGNORE_MAYBE_UNINITIALIZED_END _Pragma("GCC diagnostic pop")
#else
#define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
#define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
#define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && !defined __ICC && 6 <= __GNUC__
#define YY_IGNORE_USELESS_CAST_BEGIN \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wuseless-cast\"")
#define YY_IGNORE_USELESS_CAST_END _Pragma("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
#define YY_IGNORE_USELESS_CAST_BEGIN
#define YY_IGNORE_USELESS_CAST_END
#endif

#define YY_ASSERT(E) ((void)(0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

#ifdef YYSTACK_USE_ALLOCA
#if YYSTACK_USE_ALLOCA
#ifdef __GNUC__
#define YYSTACK_ALLOC __builtin_alloca
#elif defined __BUILTIN_VA_ARG_INCR
#include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#elif defined _AIX
#define YYSTACK_ALLOC __alloca
#elif defined _MSC_VER
#include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#define alloca _alloca
#else
#define YYSTACK_ALLOC alloca
#if !defined _ALLOCA_H && !defined EXIT_SUCCESS
#include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
/* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#endif
#endif
#endif
#endif

#ifdef YYSTACK_ALLOC
/* Pacify GCC's 'empty if-body' warning.  */
#define YYSTACK_FREE(Ptr) \
  do { /* empty */        \
    ;                     \
  } while (0)
#ifndef YYSTACK_ALLOC_MAXIMUM
/* The OS might guarantee only one guard page at the bottom of the stack,
   and a page size can be as small as 4096 bytes.  So we cannot safely
   invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
   to allow for a few compiler-allocated temporary stack slots.  */
#define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#endif
#else
#define YYSTACK_ALLOC YYMALLOC
#define YYSTACK_FREE YYFREE
#ifndef YYSTACK_ALLOC_MAXIMUM
#define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#endif
#if (defined __cplusplus && !defined EXIT_SUCCESS && \
     !((defined YYMALLOC || defined malloc) && (defined YYFREE || defined free)))
#include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#endif
#ifndef YYMALLOC
#define YYMALLOC malloc
#if !defined malloc && !defined EXIT_SUCCESS
void *malloc(YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#endif
#endif
#ifndef YYFREE
#define YYFREE free
#if !defined free && !defined EXIT_SUCCESS
void free(void *); /* INFRINGES ON USER NAME SPACE */
#endif
#endif
#endif
#endif /* !defined yyoverflow */

#if (!defined yyoverflow &&                                                        \
     (!defined __cplusplus || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL && \
                               defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc {
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
#define YYSTACK_GAP_MAXIMUM (YYSIZEOF(union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
#define YYSTACK_BYTES(N) \
  ((N) * (YYSIZEOF(yy_state_t) + YYSIZEOF(YYSTYPE) + YYSIZEOF(YYLTYPE)) + 2 * YYSTACK_GAP_MAXIMUM)

#define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
#define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
  do {                                                                 \
    YYPTRDIFF_T yynewbytes;                                            \
    YYCOPY(&yyptr->Stack_alloc, Stack, yysize);                        \
    Stack = &yyptr->Stack_alloc;                                       \
    yynewbytes = yystacksize * YYSIZEOF(*Stack) + YYSTACK_GAP_MAXIMUM; \
    yyptr += yynewbytes / YYSIZEOF(*yyptr);                            \
  } while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
#ifndef YYCOPY
#if defined __GNUC__ && 1 < __GNUC__
#define YYCOPY(Dst, Src, Count) __builtin_memcpy(Dst, Src, YY_CAST(YYSIZE_T, (Count)) * sizeof(*(Src)))
#else
#define YYCOPY(Dst, Src, Count)                                  \
  do {                                                           \
    YYPTRDIFF_T yyi;                                             \
    for (yyi = 0; yyi < (Count); yyi++) (Dst)[yyi] = (Src)[yyi]; \
  } while (0)
#endif
#endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL 2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST 415

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS 50
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS 39
/* YYNRULES -- Number of rules.  */
#define YYNRULES 115
/* YYNSTATES -- Number of states.  */
#define YYNSTATES 222

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK 282

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX) \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? YY_CAST(yysymbol_kind_t, yytranslate[YYX]) : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] = {
  0, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,  2,  34, 2,  35, 2,  36, 2,  2,  31, 32, 37, 43, 49, 44, 38, 45, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,  40, 28, 42, 33, 41, 39, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  47, 2,  48, 46, 2,  2,  2,  2,  2,  2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  29, 2,  30, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1, 2, 3, 4,
  5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] = {
  0,   172, 172, 173, 179, 183, 184, 185, 189, 191, 190, 204, 212, 219, 220, 224, 232, 237, 242, 247,
  253, 252, 262, 269, 274, 273, 286, 285, 298, 299, 300, 304, 305, 306, 314, 315, 319, 320, 321, 322,
  323, 327, 336, 337, 342, 346, 351, 356, 364, 365, 372, 373, 380, 381, 385, 392, 393, 397, 401, 405,
  412, 413, 417, 424, 425, 429, 433, 441, 442, 446, 457, 464, 465, 472, 473, 478, 482, 490, 494, 498,
  502, 506, 511, 516, 520, 524, 528, 532, 540, 543, 552, 557, 561, 566, 572, 576, 584, 585, 592, 593,
  597, 602, 610, 611, 616, 619, 623, 628, 636, 641, 650, 653, 657, 662, 670, 674};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST(yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name(yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] = {"\"end of file\"",
                                      "error",
                                      "\"invalid token\"",
                                      "TOK_ERROR",
                                      "TOK_EOT",
                                      "TOK_MODULE",
                                      "TOK_FUNCTION",
                                      "TOK_IF",
                                      "TOK_ELSE",
                                      "TOK_FOR",
                                      "TOK_LET",
                                      "TOK_ASSERT",
                                      "TOK_ECHO",
                                      "TOK_EACH",
                                      "TOK_ID",
                                      "TOK_STRING",
                                      "TOK_USE",
                                      "TOK_NUMBER",
                                      "TOK_TRUE",
                                      "TOK_FALSE",
                                      "TOK_UNDEF",
                                      "LE",
                                      "GE",
                                      "EQ",
                                      "NEQ",
                                      "AND",
                                      "OR",
                                      "NO_ELSE",
                                      "';'",
                                      "'{'",
                                      "'}'",
                                      "'('",
                                      "')'",
                                      "'='",
                                      "'!'",
                                      "'#'",
                                      "'%'",
                                      "'*'",
                                      "'.'",
                                      "'?'",
                                      "':'",
                                      "'>'",
                                      "'<'",
                                      "'+'",
                                      "'-'",
                                      "'/'",
                                      "'^'",
                                      "'['",
                                      "']'",
                                      "','",
                                      "$accept",
                                      "input",
                                      "statement",
                                      "$@1",
                                      "inner_input",
                                      "assignment",
                                      "module_instantiation",
                                      "@2",
                                      "ifelse_statement",
                                      "$@3",
                                      "if_statement",
                                      "@4",
                                      "child_statements",
                                      "child_statement",
                                      "module_id",
                                      "single_module_instantiation",
                                      "expr",
                                      "logic_or",
                                      "logic_and",
                                      "equality",
                                      "comparison",
                                      "addition",
                                      "multiplication",
                                      "unary",
                                      "exponent",
                                      "call",
                                      "primary",
                                      "expr_or_empty",
                                      "list_comprehension_elements",
                                      "list_comprehension_elements_p",
                                      "optional_trailing_comma",
                                      "vector_elements",
                                      "vector_element",
                                      "parameters",
                                      "parameter_list",
                                      "parameter",
                                      "arguments",
                                      "argument_list",
                                      "argument",
                                      YY_NULLPTR};

static const char *yysymbol_name(yysymbol_kind_t yysymbol) { return yytname[yysymbol]; }
#endif

#define YYPACT_NINF (-97)

#define yypact_value_is_default(Yyn) ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) 0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] = {
  -97, 285, -97, -97, -1,  11,  10,  -97, -97, -97, -97, -97, 19,  -97, -97, -97, 378, 378, 378,
  378, -97, -97, -97, -97, 24,  31,  -97, 42,  54,  25,  25,  72,  298, 51,  -97, -97, -97, -97,
  -97, 240, 365, 86,  86,  70,  71,  74,  76,  -97, -97, -97, -97, -97, -97, 25,  339, 339, 339,
  124, 80,  -10, 78,  27,  5,   22,  -15, -97, -97, 17,  -97, 85,  -97, -97, -97, 365, 89,  -97,
  91,  77,  -97, -97, -97, -97, -97, 92,  95,  79,  -97, 97,  86,  240, 240, 240, 108, -97, -97,
  -97, 114, 115, 116, 183, 222, -97, 117, -97, -97, 99,  -97, -97, 339, 25,  339, 339, 339, 339,
  339, 339, 339, 339, 339, 339, 339, 339, 240, 142, 339, 25,  -97, -97, 25,  -97, 240, -97, 81,
  25,  -97, 86,  -97, 126, 128, 129, 130, 132, -97, 25,  240, 240, -97, -97, 133, 25,  183, 118,
  365, 78,  139, 27,  5,   5,   22,  22,  22,  22,  -15, -15, -97, -97, -97, 137, -97, -97, 122,
  -97, -97, -97, -97, -97, -97, 332, -97, 25,  25,  25,  25,  25,  148, 21,  149, -97, -25, -97,
  -97, -97, 25,  -97, -97, -97, 154, -97, -97, -97, -97, -97, 183, 25,  183, 183, 25,  -97, -97,
  -97, 175, 156, -97, -97, 140, 183, 240, -97, -97, 153, 183, -97};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] = {
  2,   0,   1,   12,  0,   0,   0,   36,  37,  38, 39,  40, 34,  3,  5,  13,  0,  0,   0,   0,   4,
  8,   7,   22,  23,  0,   20,  0,   0,   0,   0,  0,   0,  34,  16, 17, 18,  19, 24,  110, 0,   104,
  104, 0,   0,   0,   0,   82,  81,  80,  77,  78, 79,  0,  0,   0,  0,  0,   0,  42,  48,  50,  52,
  55,  60,  63,  67,  71,  73,  0,   35,  6,   14, 0,   82, 114, 0,  98, 112, 31, 28,  33,  21,  108,
  0,   98,  106, 0,   104, 110, 110, 110, 0,   70, 68,  69, 0,   0,  0,  0,   0,  86,  103, 96,  102,
  98,  100, 26,  0,   0,   0,   0,   0,   0,   0,  0,   0,  0,   0,  0,  0,   0,  110, 0,   0,   0,
  15,  25,  0,   41,  99,  111, 0,   0,   9,   99, 105, 0,  0,   0,  0,  0,   83, 0,   110, 110, 103,
  91,  0,   0,   99,  0,   0,   49,  0,   51,  53, 54,  59, 57,  56, 58, 61,  62, 66,  64,  65,  0,
  76,  72,  0,   115, 113, 32,  30,  29,  109, 0,  107, 0,  0,   0,  88, 88,  0,  0,   0,   97,  0,
  101, 87,  27,  0,   74,  75,  10,  0,   43,  45, 89,  46, 47,  0,  0,  0,   0,  0,   84,  44,  11,
  94,  0,   92,  90,  0,   0,   110, 85,  95,  0,  0,   93};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] = {-97, -97, -28, -97, -97, 55,  1,   -97, -97, -97, -97, -97, -97,
                                      -65, -97, -97, -29, -97, 96,  98,  -34, -32, -20, -45, -97, -97,
                                      -97, 16,  105, 2,   -71, -97, -96, -30, -97, 75,  -84, -97, 82};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] = {0,  1,   20,  177, 32,  21,  81,  40, 23, 73, 24, 152, 132,
                                         82, 25,  26,  75,  59,  60,  61,  62, 63, 64, 65, 66,  67,
                                         68, 200, 103, 104, 131, 105, 106, 84, 85, 86, 76, 77,  78};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] = {
  58,  69,  22,  147, 72,  139, 140, 141, 127, 93,  94,  95,  87,  27,  136, 206, 108, 34,  35,  36,
  37,  119, 120, 207, 92,  28,  113, 114, 102, 109, 121, 43,  38,  22,  151, 44,  45,  46,  167, 47,
  48,  29,  49,  50,  51,  52,  115, 116, 122, 203, 111, 112, 30,  204, 189, 123, 53,  31,  138, 54,
  185, 186, 39,  124, 125, 117, 118, 175, 55,  56,  146, 92,  57,  41,  164, 165, 166, 156, 157, 169,
  154, 158, 159, 160, 161, 42,  70,  191, 6,   31,  7,   8,   9,   10,  11,  12,  170, 162, 163, 171,
  83,  88,  89,  110, 176, 90,  210, 91,  212, 79,  80,  173, 107, 126, 184, 16,  17,  18,  19,  218,
  188, 146, 128, 129, 221, 133, 130, 134, 135, 137, 43,  96,  219, 97,  98,  45,  46,  99,  47,  48,
  142, 49,  50,  51,  52,  143, 144, 145, 150, 195, 196, 197, 198, 199, 199, 100, 168, 149, 54,  179,
  180, 181, 182, 208, 183, 187, 190, 55,  56,  193, 194, 57,  101, 146, 211, 146, 198, 214, 22,  192,
  202, 205, 209, 215, 216, 220, 146, 174, 217, 43,  96,  146, 97,  98,  45,  46,  99,  47,  48,  201,
  49,  50,  51,  52,  153, 148, 0,   213, 155, 0,   178, 0,   172, 0,   100, 0,   0,   54,  0,   0,
  0,   0,   0,   0,   0,   0,   55,  56,  43,  96,  57,  97,  98,  45,  46,  99,  47,  48,  0,   49,
  50,  51,  52,  0,   0,   0,   43,  0,   0,   0,   44,  45,  46,  53,  74,  48,  54,  49,  50,  51,
  52,  0,   0,   0,   0,   55,  56,  0,   0,   57,  0,   53,  0,   0,   54,  0,   0,   0,   0,   0,
  0,   0,   0,   55,  56,  2,   0,   57,  0,   3,   4,   5,   6,   0,   7,   8,   9,   10,  11,  12,
  0,   13,  3,   4,   5,   6,   0,   7,   8,   9,   10,  11,  12,  14,  15,  0,   0,   0,   0,   16,
  17,  18,  19,  0,   0,   0,   14,  15,  71,  0,   0,   0,   16,  17,  18,  19,  3,   4,   5,   6,
  0,   7,   8,   9,   10,  11,  12,  0,   0,   0,   0,   0,   0,   47,  48,  0,   49,  50,  51,  52,
  14,  15,  0,   0,   0,   0,   16,  17,  18,  19,  53,  0,   6,   54,  7,   8,   9,   10,  11,  33,
  0,   0,   55,  56,  0,   6,   57,  7,   8,   9,   10,  11,  33,  79,  80,  0,   0,   0,   0,   16,
  17,  18,  19,  0,   0,   0,   0,   0,   0,   0,   0,   0,   16,  17,  18,  19};

static const yytype_int16 yycheck[] = {
  29,  30,  1,   99,  32,  89,  90,  91,  73,  54,  55,  56,  42,  14,  85,  40,  26,  16,  17,  18,
  19,  36,  37,  48,  53,  14,  21,  22,  57,  39,  45,  6,   8,   32,  105, 10,  11,  12,  122, 14,
  15,  31,  17,  18,  19,  20,  41,  42,  31,  28,  23,  24,  33,  32,  150, 38,  31,  38,  88,  34,
  144, 145, 31,  46,  47,  43,  44,  132, 43,  44,  99,  100, 47,  31,  119, 120, 121, 111, 112, 124,
  109, 113, 114, 115, 116, 31,  14,  152, 7,   38,  9,   10,  11,  12,  13,  14,  125, 117, 118, 128,
  14,  31,  31,  25,  133, 31,  202, 31,  204, 28,  29,  30,  32,  28,  143, 34,  35,  36,  37,  215,
  149, 150, 33,  32,  220, 33,  49,  32,  49,  32,  6,   7,   216, 9,   10,  11,  12,  13,  14,  15,
  32,  17,  18,  19,  20,  31,  31,  31,  49,  177, 179, 180, 181, 182, 183, 31,  14,  40,  34,  33,
  32,  32,  32,  192, 32,  32,  48,  43,  44,  32,  48,  47,  48,  202, 203, 204, 205, 206, 177, 40,
  32,  32,  28,  8,   28,  32,  215, 132, 48,  6,   7,   220, 9,   10,  11,  12,  13,  14,  15,  183,
  17,  18,  19,  20,  108, 100, -1,  205, 110, -1,  135, -1,  130, -1,  31,  -1,  -1,  34,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  43,  44,  6,   7,   47,  9,   10,  11,  12,  13,  14,  15,  -1,  17,
  18,  19,  20,  -1,  -1,  -1,  6,   -1,  -1,  -1,  10,  11,  12,  31,  14,  15,  34,  17,  18,  19,
  20,  -1,  -1,  -1,  -1,  43,  44,  -1,  -1,  47,  -1,  31,  -1,  -1,  34,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  43,  44,  0,   -1,  47,  -1,  4,   5,   6,   7,   -1,  9,   10,  11,  12,  13,  14,
  -1,  16,  4,   5,   6,   7,   -1,  9,   10,  11,  12,  13,  14,  28,  29,  -1,  -1,  -1,  -1,  34,
  35,  36,  37,  -1,  -1,  -1,  28,  29,  30,  -1,  -1,  -1,  34,  35,  36,  37,  4,   5,   6,   7,
  -1,  9,   10,  11,  12,  13,  14,  -1,  -1,  -1,  -1,  -1,  -1,  14,  15,  -1,  17,  18,  19,  20,
  28,  29,  -1,  -1,  -1,  -1,  34,  35,  36,  37,  31,  -1,  7,   34,  9,   10,  11,  12,  13,  14,
  -1,  -1,  43,  44,  -1,  7,   47,  9,   10,  11,  12,  13,  14,  28,  29,  -1,  -1,  -1,  -1,  34,
  35,  36,  37,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  34,  35,  36,  37};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] = {
  0,  51, 0,  4,  5,  6,  7,  9,  10, 11, 12, 13, 14, 16, 28, 29, 34, 35, 36, 37, 52, 55, 56, 58, 60,
  64, 65, 14, 14, 31, 33, 38, 54, 14, 56, 56, 56, 56, 8,  31, 57, 31, 31, 6,  10, 11, 12, 14, 15, 17,
  18, 19, 20, 31, 34, 43, 44, 47, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 66, 14, 30, 52, 59, 14,
  66, 86, 87, 88, 28, 29, 56, 63, 14, 83, 84, 85, 83, 31, 31, 31, 31, 66, 73, 73, 73, 7,  9,  10, 13,
  31, 48, 66, 78, 79, 81, 82, 32, 26, 39, 25, 23, 24, 21, 22, 41, 42, 43, 44, 36, 37, 45, 31, 38, 46,
  47, 28, 63, 33, 32, 49, 80, 62, 33, 32, 49, 80, 32, 83, 86, 86, 86, 32, 31, 31, 31, 66, 82, 78, 40,
  49, 80, 61, 68, 66, 69, 70, 70, 71, 71, 71, 71, 72, 72, 73, 73, 73, 86, 14, 73, 66, 66, 88, 30, 55,
  63, 66, 53, 85, 33, 32, 32, 32, 32, 66, 86, 86, 32, 66, 82, 48, 63, 40, 32, 48, 52, 66, 66, 66, 66,
  77, 77, 32, 28, 32, 32, 40, 48, 66, 28, 82, 66, 82, 79, 66, 8,  28, 48, 82, 86, 32, 82};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] = {
  0,  50, 51, 51, 51, 52, 52, 52, 52, 53, 52, 52, 52, 54, 54, 55, 56, 56, 56, 56, 57, 56, 56, 58,
  59, 58, 61, 60, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64, 64, 64, 65, 66, 66, 66, 66, 66, 66,
  67, 67, 68, 68, 69, 69, 69, 70, 70, 70, 70, 70, 71, 71, 71, 72, 72, 72, 72, 73, 73, 73, 73, 74,
  74, 75, 75, 75, 75, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 77, 77, 78, 78, 78, 78, 78, 78,
  79, 79, 80, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 88, 88};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] = {
  0, 2, 0, 2, 2, 1, 3, 1, 1, 0, 7, 8, 1, 0, 2, 4, 2, 2, 2, 2, 0, 3, 1, 1, 0, 4, 0, 6, 0,
  2, 2, 1, 3, 1, 1, 3, 1, 1, 1, 1, 1, 4, 1, 5, 5, 5, 5, 5, 1, 3, 1, 3, 1, 3, 3, 1, 3, 3,
  3, 3, 1, 3, 3, 1, 3, 3, 3, 1, 2, 2, 2, 1, 3, 1, 4, 4, 3, 1, 1, 1, 1, 1, 1, 3, 5, 7, 2,
  4, 0, 1, 5, 2, 5, 9, 5, 7, 1, 3, 0, 1, 1, 3, 1, 1, 0, 2, 1, 3, 1, 3, 0, 2, 1, 3, 1, 3};

enum { YYENOMEM = -2 };

#define yyerrok (yyerrstatus = 0)
#define yyclearin (yychar = YYEMPTY)

#define YYACCEPT goto yyacceptlab
#define YYABORT goto yyabortlab
#define YYERROR goto yyerrorlab
#define YYNOMEM goto yyexhaustedlab

#define YYRECOVERING() (!!yyerrstatus)

#define YYBACKUP(Token, Value)                      \
  do                                                \
    if (yychar == YYEMPTY) {                        \
      yychar = (Token);                             \
      yylval = (Value);                             \
      YYPOPSTACK(yylen);                            \
      yystate = *yyssp;                             \
      goto yybackup;                                \
    } else {                                        \
      yyerror(YY_("syntax error: cannot back up")); \
      YYERROR;                                      \
    }                                               \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
#define YYLLOC_DEFAULT(Current, Rhs, N)                                              \
  do                                                                                 \
    if (N) {                                                                         \
      (Current).first_line = YYRHSLOC(Rhs, 1).first_line;                            \
      (Current).first_column = YYRHSLOC(Rhs, 1).first_column;                        \
      (Current).last_line = YYRHSLOC(Rhs, N).last_line;                              \
      (Current).last_column = YYRHSLOC(Rhs, N).last_column;                          \
    } else {                                                                         \
      (Current).first_line = (Current).last_line = YYRHSLOC(Rhs, 0).last_line;       \
      (Current).first_column = (Current).last_column = YYRHSLOC(Rhs, 0).last_column; \
    }                                                                                \
  while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])

/* Enable debugging if requested.  */
#if YYDEBUG

#ifndef YYFPRINTF
#include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#define YYFPRINTF fprintf
#endif

#define YYDPRINTF(Args)          \
  do {                           \
    if (yydebug) YYFPRINTF Args; \
  } while (0)

/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YYLOCATION_PRINT

#if defined YY_LOCATION_PRINT

/* Temporary convenience wrapper in case some people defined the
   undocumented and private YY_LOCATION_PRINT macros.  */
#define YYLOCATION_PRINT(File, Loc) YY_LOCATION_PRINT(File, *(Loc))

#elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int yy_location_print_(FILE *yyo, YYLTYPE const *const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line) {
    res += YYFPRINTF(yyo, "%d", yylocp->first_line);
    if (0 <= yylocp->first_column) res += YYFPRINTF(yyo, ".%d", yylocp->first_column);
  }
  if (0 <= yylocp->last_line) {
    if (yylocp->first_line < yylocp->last_line) {
      res += YYFPRINTF(yyo, "-%d", yylocp->last_line);
      if (0 <= end_col) res += YYFPRINTF(yyo, ".%d", end_col);
    } else if (0 <= end_col && yylocp->first_column < end_col) res += YYFPRINTF(yyo, "-%d", end_col);
  }
  return res;
}

#define YYLOCATION_PRINT yy_location_print_

/* Temporary convenience wrapper in case some people defined the
   undocumented and private YY_LOCATION_PRINT macros.  */
#define YY_LOCATION_PRINT(File, Loc) YYLOCATION_PRINT(File, &(Loc))

#else

#define YYLOCATION_PRINT(File, Loc) ((void)0)
/* Temporary convenience wrapper in case some people defined the
   undocumented and private YY_LOCATION_PRINT macros.  */
#define YY_LOCATION_PRINT YYLOCATION_PRINT

#endif
#endif /* !defined YYLOCATION_PRINT */

#define YY_SYMBOL_PRINT(Title, Kind, Value, Location) \
  do {                                                \
    if (yydebug) {                                    \
      YYFPRINTF(stderr, "%s ", Title);                \
      yy_symbol_print(stderr, Kind, Value, Location); \
      YYFPRINTF(stderr, "\n");                        \
    }                                                 \
  } while (0)

/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void yy_symbol_value_print(FILE *yyo, yysymbol_kind_t yykind, YYSTYPE const *const yyvaluep,
                                  YYLTYPE const *const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE(yyoutput);
  YY_USE(yylocationp);
  if (!yyvaluep) return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE(yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void yy_symbol_print(FILE *yyo, yysymbol_kind_t yykind, YYSTYPE const *const yyvaluep,
                            YYLTYPE const *const yylocationp)
{
  YYFPRINTF(yyo, "%s %s (", yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name(yykind));

  YYLOCATION_PRINT(yyo, yylocationp);
  YYFPRINTF(yyo, ": ");
  yy_symbol_value_print(yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF(yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void yy_stack_print(yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF(stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++) {
    int yybot = *yybottom;
    YYFPRINTF(stderr, " %d", yybot);
  }
  YYFPRINTF(stderr, "\n");
}

#define YY_STACK_PRINT(Bottom, Top)               \
  do {                                            \
    if (yydebug) yy_stack_print((Bottom), (Top)); \
  } while (0)

/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void yy_reduce_print(yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF(stderr, "Reducing stack by rule %d (line %d):\n", yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++) {
    YYFPRINTF(stderr, "   $%d = ", yyi + 1);
    yy_symbol_print(stderr, YY_ACCESSING_SYMBOL(+yyssp[yyi + 1 - yynrhs]), &yyvsp[(yyi + 1) - (yynrhs)],
                    &(yylsp[(yyi + 1) - (yynrhs)]));
    YYFPRINTF(stderr, "\n");
  }
}

#define YY_REDUCE_PRINT(Rule)                                \
  do {                                                       \
    if (yydebug) yy_reduce_print(yyssp, yyvsp, yylsp, Rule); \
  } while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
#define YYDPRINTF(Args) ((void)0)
#define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
#define YY_STACK_PRINT(Bottom, Top)
#define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void yydestruct(const char *yymsg, yysymbol_kind_t yykind, YYSTYPE *yyvaluep,
                       YYLTYPE *yylocationp)
{
  YY_USE(yyvaluep);
  YY_USE(yylocationp);
  if (!yymsg) yymsg = "Deleting";
  YY_SYMBOL_PRINT(yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE(yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = {1, 1, 1, 1}
#endif
;
/* Number of syntax errors so far.  */
int yynerrs;

/*----------.
| yyparse.  |
`----------*/

int yyparse(void)
{
  yy_state_fast_t yystate = 0;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus = 0;

  /* Refer to the stacks through separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* Their size.  */
  YYPTRDIFF_T yystacksize = YYINITDEPTH;

  /* The state stack: array, bottom, top.  */
  yy_state_t yyssa[YYINITDEPTH];
  yy_state_t *yyss = yyssa;
  yy_state_t *yyssp = yyss;

  /* The semantic value stack: array, bottom, top.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp = yyvs;

  /* The location stack: array, bottom, top.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

#define YYPOPSTACK(N) (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

/* User initialization code.  */
#line 89 "src/core/parser.y"
  {
    yylloc.first_line = 1;
    yylloc.first_column = 1;
    yylloc.last_column = 1;
    yylloc.last_line = 1;
  }

#line 1319 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"

  yylsp[0] = yylloc;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF((stderr, "Entering state %d\n", yystate));
  YY_ASSERT(0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST(yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT(yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
  {
    /* Get the current used size of the three stacks, in elements.  */
    YYPTRDIFF_T yysize = yyssp - yyss + 1;

#if defined yyoverflow
    {
      /* Give user a chance to reallocate the stack.  Use copies of
         these so that the &'s don't force the real ones into
         memory.  */
      yy_state_t *yyss1 = yyss;
      YYSTYPE *yyvs1 = yyvs;
      YYLTYPE *yyls1 = yyls;

      /* Each stack pointer address is followed by the size of the
         data in use in that stack, in bytes.  This used to be a
         conditional around just the two extra args, but that might
         be undefined if yyoverflow is a macro.  */
      yyoverflow(YY_("memory exhausted"), &yyss1, yysize * YYSIZEOF(*yyssp), &yyvs1,
                 yysize * YYSIZEOF(*yyvsp), &yyls1, yysize * YYSIZEOF(*yylsp), &yystacksize);
      yyss = yyss1;
      yyvs = yyvs1;
      yyls = yyls1;
    }
#else /* defined YYSTACK_RELOCATE */
    /* Extend the stack our own way.  */
    if (YYMAXDEPTH <= yystacksize) YYNOMEM;
    yystacksize *= 2;
    if (YYMAXDEPTH < yystacksize) yystacksize = YYMAXDEPTH;

    {
      yy_state_t *yyss1 = yyss;
      union yyalloc *yyptr =
        YY_CAST(union yyalloc *, YYSTACK_ALLOC(YY_CAST(YYSIZE_T, YYSTACK_BYTES(yystacksize))));
      if (!yyptr) YYNOMEM;
      YYSTACK_RELOCATE(yyss_alloc, yyss);
      YYSTACK_RELOCATE(yyvs_alloc, yyvs);
      YYSTACK_RELOCATE(yyls_alloc, yyls);
#undef YYSTACK_RELOCATE
      if (yyss1 != yyssa) YYSTACK_FREE(yyss1);
    }
#endif

    yyssp = yyss + yysize - 1;
    yyvsp = yyvs + yysize - 1;
    yylsp = yyls + yysize - 1;

    YY_IGNORE_USELESS_CAST_BEGIN
    YYDPRINTF((stderr, "Stack size increased to %ld\n", YY_CAST(long, yystacksize)));
    YY_IGNORE_USELESS_CAST_END

    if (yyss + yystacksize - 1 <= yyssp) YYABORT;
  }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL) YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default(yyn)) goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY) {
    YYDPRINTF((stderr, "Reading a token\n"));
    yychar = yylex();
  }

  if (yychar <= YYEOF) {
    yychar = YYEOF;
    yytoken = YYSYMBOL_YYEOF;
    YYDPRINTF((stderr, "Now at end of input.\n"));
  } else if (yychar == YYerror) {
    /* The scanner already issued an error message, process directly
       to error recovery.  But do not keep the error token as
       lookahead, it is too special and may lead us to an endless
       loop in error recovery. */
    yychar = YYUNDEF;
    yytoken = YYSYMBOL_YYerror;
    yyerror_range[1] = yylloc;
    goto yyerrlab1;
  } else {
    yytoken = YYTRANSLATE(yychar);
    YY_SYMBOL_PRINT("Next token is", yytoken, &yylval, &yylloc);
  }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken) goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0) {
    if (yytable_value_is_error(yyn)) goto yyerrlab;
    yyn = -yyn;
    goto yyreduce;
  }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus) yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;

/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0) goto yyerrlab;
  goto yyreduce;

/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1 - yylen];

  /* Default location. */
  YYLLOC_DEFAULT(yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT(yyn);
  switch (yyn) {
  case 3: /* input: input TOK_USE  */
#line 175 "src/core/parser.y"
  {
    rootfile->registerUse(std::string((yyvsp[0].text)),
                          lexer_is_main_file() && parsingMainFile ? LOC((yylsp[0])) : Location::NONE);
    free((yyvsp[0].text));
  }
#line 1535 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 7: /* statement: module_instantiation  */
#line 186 "src/core/parser.y"
  {
    if ((yyvsp[0].inst))
      scope_stack.top()->addModuleInst(std::shared_ptr<ModuleInstantiation>((yyvsp[0].inst)));
  }
#line 1543 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 9: /* $@1: %empty  */
#line 191 "src/core/parser.y"
  {
    UserModule *newmodule = new UserModule((yyvsp[-3].text), LOCD("module", (yyloc)));
    newmodule->parameters = *(yyvsp[-1].args);
    auto top = scope_stack.top();
    scope_stack.push(&newmodule->body);
    top->addModule(std::shared_ptr<UserModule>(newmodule));
    free((yyvsp[-3].text));
    delete (yyvsp[-1].args);
  }
#line 1557 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 10: /* statement: TOK_MODULE TOK_ID '(' parameters ')' $@1 statement  */
#line 201 "src/core/parser.y"
  {
    scope_stack.pop();
  }
#line 1565 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 11: /* statement: TOK_FUNCTION TOK_ID '(' parameters ')' '=' expr ';'  */
#line 205 "src/core/parser.y"
  {
    scope_stack.top()->addFunction(std::make_shared<UserFunction>(
      (yyvsp[-6].text), *(yyvsp[-4].args), std::shared_ptr<Expression>((yyvsp[-1].expr)),
      LOCD("function", (yyloc))));
    free((yyvsp[-6].text));
    delete (yyvsp[-4].args);
  }
#line 1577 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 12: /* statement: TOK_EOT  */
#line 213 "src/core/parser.y"
  {
    fileEnded = true;
  }
#line 1585 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 15: /* assignment: TOK_ID '=' expr ';'  */
#line 225 "src/core/parser.y"
  {
    handle_assignment((yyvsp[-3].text), (yyvsp[-1].expr), LOCD("assignment", (yyloc)));
    free((yyvsp[-3].text));
  }
#line 1594 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 16: /* module_instantiation: '!' module_instantiation  */
#line 233 "src/core/parser.y"
  {
    (yyval.inst) = (yyvsp[0].inst);
    if ((yyval.inst)) (yyval.inst)->tag_root = true;
  }
#line 1603 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 17: /* module_instantiation: '#' module_instantiation  */
#line 238 "src/core/parser.y"
  {
    (yyval.inst) = (yyvsp[0].inst);
    if ((yyval.inst)) (yyval.inst)->tag_highlight = true;
  }
#line 1612 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 18: /* module_instantiation: '%' module_instantiation  */
#line 243 "src/core/parser.y"
  {
    (yyval.inst) = (yyvsp[0].inst);
    if ((yyval.inst)) (yyval.inst)->tag_background = true;
  }
#line 1621 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 19: /* module_instantiation: '*' module_instantiation  */
#line 248 "src/core/parser.y"
  {
    delete (yyvsp[0].inst);
    (yyval.inst) = NULL;
  }
#line 1630 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 20: /* @2: %empty  */
#line 253 "src/core/parser.y"
  {
    (yyval.inst) = (yyvsp[0].inst);
    scope_stack.push(&(yyvsp[0].inst)->scope);
  }
#line 1639 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 21: /* module_instantiation: single_module_instantiation @2 child_statement  */
#line 258 "src/core/parser.y"
  {
    scope_stack.pop();
    (yyval.inst) = (yyvsp[-1].inst);
  }
#line 1648 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 22: /* module_instantiation: ifelse_statement  */
#line 263 "src/core/parser.y"
  {
    (yyval.inst) = (yyvsp[0].ifelse);
  }
#line 1656 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 23: /* ifelse_statement: if_statement  */
#line 270 "src/core/parser.y"
  {
    (yyval.ifelse) = (yyvsp[0].ifelse);
  }
#line 1664 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 24: /* $@3: %empty  */
#line 274 "src/core/parser.y"
  {
    scope_stack.push((yyvsp[-1].ifelse)->makeElseScope());
  }
#line 1672 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 25: /* ifelse_statement: if_statement TOK_ELSE $@3 child_statement  */
#line 278 "src/core/parser.y"
  {
    scope_stack.pop();
    (yyval.ifelse) = (yyvsp[-3].ifelse);
  }
#line 1681 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 26: /* @4: %empty  */
#line 286 "src/core/parser.y"
  {
    (yyval.ifelse) =
      new IfElseModuleInstantiation(std::shared_ptr<Expression>((yyvsp[-1].expr)), LOCD("if", (yyloc)));
    scope_stack.push(&(yyval.ifelse)->scope);
  }
#line 1690 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 27: /* if_statement: TOK_IF '(' expr ')' @4 child_statement  */
#line 291 "src/core/parser.y"
  {
    scope_stack.pop();
    (yyval.ifelse) = (yyvsp[-1].ifelse);
  }
#line 1699 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 33: /* child_statement: module_instantiation  */
#line 307 "src/core/parser.y"
  {
    if ((yyvsp[0].inst))
      scope_stack.top()->addModuleInst(std::shared_ptr<ModuleInstantiation>((yyvsp[0].inst)));
  }
#line 1707 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 34: /* module_id: TOK_ID  */
#line 314 "src/core/parser.y"
  {
    (yyval.text) = (yyvsp[0].text);
  }
#line 1713 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 35: /* module_id: TOK_ID '.' TOK_ID  */
#line 315 "src/core/parser.y"
  {
    char tmp[100];
    snprintf(tmp, sizeof(tmp), "(%s.%s)", (yyvsp[-2].text), (yyvsp[0].text));
    (yyval.text) = strdup(tmp);
  }
#line 1722 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 36: /* module_id: TOK_FOR  */
#line 319 "src/core/parser.y"
  {
    (yyval.text) = strdup("for");
  }
#line 1728 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 37: /* module_id: TOK_LET  */
#line 320 "src/core/parser.y"
  {
    (yyval.text) = strdup("let");
  }
#line 1734 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 38: /* module_id: TOK_ASSERT  */
#line 321 "src/core/parser.y"
  {
    (yyval.text) = strdup("assert");
  }
#line 1740 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 39: /* module_id: TOK_ECHO  */
#line 322 "src/core/parser.y"
  {
    (yyval.text) = strdup("echo");
  }
#line 1746 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 40: /* module_id: TOK_EACH  */
#line 323 "src/core/parser.y"
  {
    (yyval.text) = strdup("each");
  }
#line 1752 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 41: /* single_module_instantiation: module_id '(' arguments ')'  */
#line 328 "src/core/parser.y"
  {
    (yyval.inst) =
      new ModuleInstantiation((yyvsp[-3].text), *(yyvsp[-1].args), LOCD("modulecall", (yyloc)));
    free((yyvsp[-3].text));
    delete (yyvsp[-1].args);
  }
#line 1762 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 43: /* expr: TOK_FUNCTION '(' parameters ')' expr  */
#line 338 "src/core/parser.y"
  {
    (yyval.expr) = new FunctionDefinition((yyvsp[0].expr), *(yyvsp[-2].args), LOCD("anonfunc", (yyloc)));
    delete (yyvsp[-2].args);
  }
#line 1771 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 44: /* expr: logic_or '?' expr ':' expr  */
#line 343 "src/core/parser.y"
  {
    (yyval.expr) =
      new TernaryOp((yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr), LOCD("ternary", (yyloc)));
  }
#line 1779 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 45: /* expr: TOK_LET '(' arguments ')' expr  */
#line 347 "src/core/parser.y"
  {
    (yyval.expr) = FunctionCall::create("let", *(yyvsp[-2].args), (yyvsp[0].expr), LOCD("let", (yyloc)));
    delete (yyvsp[-2].args);
  }
#line 1788 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 46: /* expr: TOK_ASSERT '(' arguments ')' expr_or_empty  */
#line 352 "src/core/parser.y"
  {
    (yyval.expr) =
      FunctionCall::create("assert", *(yyvsp[-2].args), (yyvsp[0].expr), LOCD("assert", (yyloc)));
    delete (yyvsp[-2].args);
  }
#line 1797 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 47: /* expr: TOK_ECHO '(' arguments ')' expr_or_empty  */
#line 357 "src/core/parser.y"
  {
    (yyval.expr) =
      FunctionCall::create("echo", *(yyvsp[-2].args), (yyvsp[0].expr), LOCD("echo", (yyloc)));
    delete (yyvsp[-2].args);
  }
#line 1806 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 49: /* logic_or: logic_or OR logic_and  */
#line 366 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::LogicalOr, (yyvsp[0].expr), LOCD("or", (yyloc)));
  }
#line 1814 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 51: /* logic_and: logic_and AND equality  */
#line 374 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::LogicalAnd, (yyvsp[0].expr), LOCD("and", (yyloc)));
  }
#line 1822 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 53: /* equality: equality EQ comparison  */
#line 382 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::Equal, (yyvsp[0].expr), LOCD("equal", (yyloc)));
  }
#line 1830 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 54: /* equality: equality NEQ comparison  */
#line 386 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::NotEqual, (yyvsp[0].expr), LOCD("notequal", (yyloc)));
  }
#line 1838 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 56: /* comparison: comparison '>' addition  */
#line 394 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::Greater, (yyvsp[0].expr), LOCD("greater", (yyloc)));
  }
#line 1846 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 57: /* comparison: comparison GE addition  */
#line 398 "src/core/parser.y"
  {
    (yyval.expr) = new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::GreaterEqual, (yyvsp[0].expr),
                                LOCD("greaterequal", (yyloc)));
  }
#line 1854 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 58: /* comparison: comparison '<' addition  */
#line 402 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::Less, (yyvsp[0].expr), LOCD("less", (yyloc)));
  }
#line 1862 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 59: /* comparison: comparison LE addition  */
#line 406 "src/core/parser.y"
  {
    (yyval.expr) = new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::LessEqual, (yyvsp[0].expr),
                                LOCD("lessequal", (yyloc)));
  }
#line 1870 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 61: /* addition: addition '+' multiplication  */
#line 414 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::Plus, (yyvsp[0].expr), LOCD("addition", (yyloc)));
  }
#line 1878 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 62: /* addition: addition '-' multiplication  */
#line 418 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::Minus, (yyvsp[0].expr), LOCD("subtraction", (yyloc)));
  }
#line 1886 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 64: /* multiplication: multiplication '*' unary  */
#line 426 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::Multiply, (yyvsp[0].expr), LOCD("multiply", (yyloc)));
  }
#line 1894 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 65: /* multiplication: multiplication '/' unary  */
#line 430 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::Divide, (yyvsp[0].expr), LOCD("divide", (yyloc)));
  }
#line 1902 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 66: /* multiplication: multiplication '%' unary  */
#line 434 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::Modulo, (yyvsp[0].expr), LOCD("modulo", (yyloc)));
  }
#line 1910 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 68: /* unary: '+' unary  */
#line 443 "src/core/parser.y"
  {
    (yyval.expr) = (yyvsp[0].expr);
  }
#line 1918 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 69: /* unary: '-' unary  */
#line 447 "src/core/parser.y"
  {
    Literal *argument = dynamic_cast<Literal *>((yyvsp[0].expr));
    if (argument && argument->isDouble()) {
      double value = argument->toDouble();
      delete (yyvsp[0].expr);
      (yyval.expr) = new Literal(-value, LOCD("literal", (yyloc)));
    } else {
      (yyval.expr) = new UnaryOp(UnaryOp::Op::Negate, (yyvsp[0].expr), LOCD("negate", (yyloc)));
    }
  }
#line 1933 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 70: /* unary: '!' unary  */
#line 458 "src/core/parser.y"
  {
    (yyval.expr) = new UnaryOp(UnaryOp::Op::Not, (yyvsp[0].expr), LOCD("not", (yyloc)));
  }
#line 1941 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 72: /* exponent: call '^' unary  */
#line 466 "src/core/parser.y"
  {
    (yyval.expr) =
      new BinaryOp((yyvsp[-2].expr), BinaryOp::Op::Exponent, (yyvsp[0].expr), LOCD("exponent", (yyloc)));
  }
#line 1949 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 74: /* call: call '(' arguments ')'  */
#line 474 "src/core/parser.y"
  {
    (yyval.expr) = new FunctionCall((yyvsp[-3].expr), *(yyvsp[-1].args), LOCD("functioncall", (yyloc)));
    delete (yyvsp[-1].args);
  }
#line 1958 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 75: /* call: call '[' expr ']'  */
#line 479 "src/core/parser.y"
  {
    (yyval.expr) = new ArrayLookup((yyvsp[-3].expr), (yyvsp[-1].expr), LOCD("index", (yyloc)));
  }
#line 1966 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 76: /* call: call '.' TOK_ID  */
#line 483 "src/core/parser.y"
  {
    (yyval.expr) = new MemberLookup((yyvsp[-2].expr), (yyvsp[0].text), LOCD("member", (yyloc)));
    free((yyvsp[0].text));
  }
#line 1975 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 77: /* primary: TOK_TRUE  */
#line 491 "src/core/parser.y"
  {
    (yyval.expr) = new Literal(true, LOCD("literal", (yyloc)));
  }
#line 1983 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 78: /* primary: TOK_FALSE  */
#line 495 "src/core/parser.y"
  {
    (yyval.expr) = new Literal(false, LOCD("literal", (yyloc)));
  }
#line 1991 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 79: /* primary: TOK_UNDEF  */
#line 499 "src/core/parser.y"
  {
    (yyval.expr) = new Literal(LOCD("literal", (yyloc)));
  }
#line 1999 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 80: /* primary: TOK_NUMBER  */
#line 503 "src/core/parser.y"
  {
    (yyval.expr) = new Literal((yyvsp[0].number), LOCD("literal", (yyloc)));
  }
#line 2007 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 81: /* primary: TOK_STRING  */
#line 507 "src/core/parser.y"
  {
    (yyval.expr) = new Literal(std::string((yyvsp[0].text)), LOCD("string", (yyloc)));
    free((yyvsp[0].text));
  }
#line 2016 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 82: /* primary: TOK_ID  */
#line 512 "src/core/parser.y"
  {
    (yyval.expr) = new Lookup((yyvsp[0].text), LOCD("variable", (yyloc)));
    free((yyvsp[0].text));
  }
#line 2025 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 83: /* primary: '(' expr ')'  */
#line 517 "src/core/parser.y"
  {
    (yyval.expr) = (yyvsp[-1].expr);
  }
#line 2033 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 84: /* primary: '[' expr ':' expr ']'  */
#line 521 "src/core/parser.y"
  {
    (yyval.expr) = new Range((yyvsp[-3].expr), (yyvsp[-1].expr), LOCD("range", (yyloc)));
  }
#line 2041 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 85: /* primary: '[' expr ':' expr ':' expr ']'  */
#line 525 "src/core/parser.y"
  {
    (yyval.expr) =
      new Range((yyvsp[-5].expr), (yyvsp[-3].expr), (yyvsp[-1].expr), LOCD("range", (yyloc)));
  }
#line 2049 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 86: /* primary: '[' ']'  */
#line 529 "src/core/parser.y"
  {
    (yyval.expr) = new Vector(LOCD("vector", (yyloc)));
  }
#line 2057 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 87: /* primary: '[' vector_elements optional_trailing_comma ']'  */
#line 533 "src/core/parser.y"
  {
    (yyval.expr) = (yyvsp[-2].vec);
  }
#line 2065 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 88: /* expr_or_empty: %empty  */
#line 540 "src/core/parser.y"
  {
    (yyval.expr) = NULL;
  }
#line 2073 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 89: /* expr_or_empty: expr  */
#line 544 "src/core/parser.y"
  {
    (yyval.expr) = (yyvsp[0].expr);
  }
#line 2081 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 90: /* list_comprehension_elements: TOK_LET '(' arguments ')' list_comprehension_elements_p  */
#line 553 "src/core/parser.y"
  {
    (yyval.expr) = new LcLet(*(yyvsp[-2].args), (yyvsp[0].expr), LOCD("lclet", (yyloc)));
    delete (yyvsp[-2].args);
  }
#line 2090 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 91: /* list_comprehension_elements: TOK_EACH vector_element  */
#line 558 "src/core/parser.y"
  {
    (yyval.expr) = new LcEach((yyvsp[0].expr), LOCD("lceach", (yyloc)));
  }
#line 2098 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 92: /* list_comprehension_elements: TOK_FOR '(' arguments ')' vector_element  */
#line 562 "src/core/parser.y"
  {
    (yyval.expr) = new LcFor(*(yyvsp[-2].args), (yyvsp[0].expr), LOCD("lcfor", (yyloc)));
    delete (yyvsp[-2].args);
  }
#line 2107 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 93: /* list_comprehension_elements: TOK_FOR '(' arguments ';' expr ';' arguments ')'
              vector_element  */
#line 567 "src/core/parser.y"
  {
    (yyval.expr) = new LcForC(*(yyvsp[-6].args), *(yyvsp[-2].args), (yyvsp[-4].expr), (yyvsp[0].expr),
                              LOCD("lcforc", (yyloc)));
    delete (yyvsp[-6].args);
    delete (yyvsp[-2].args);
  }
#line 2117 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 94: /* list_comprehension_elements: TOK_IF '(' expr ')' vector_element  */
#line 573 "src/core/parser.y"
  {
    (yyval.expr) = new LcIf((yyvsp[-2].expr), (yyvsp[0].expr), 0, LOCD("lcif", (yyloc)));
  }
#line 2125 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 95: /* list_comprehension_elements: TOK_IF '(' expr ')' vector_element TOK_ELSE vector_element  */
#line 577 "src/core/parser.y"
  {
    (yyval.expr) =
      new LcIf((yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr), LOCD("lcifelse", (yyloc)));
  }
#line 2133 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 97: /* list_comprehension_elements_p: '(' list_comprehension_elements ')'  */
#line 586 "src/core/parser.y"
  {
    (yyval.expr) = (yyvsp[-1].expr);
  }
#line 2141 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 100: /* vector_elements: vector_element  */
#line 598 "src/core/parser.y"
  {
    (yyval.vec) = new Vector(LOCD("vector", (yyloc)));
    (yyval.vec)->emplace_back((yyvsp[0].expr));
  }
#line 2150 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 101: /* vector_elements: vector_elements ',' vector_element  */
#line 603 "src/core/parser.y"
  {
    (yyval.vec) = (yyvsp[-2].vec);
    (yyval.vec)->emplace_back((yyvsp[0].expr));
  }
#line 2159 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 104: /* parameters: %empty  */
#line 616 "src/core/parser.y"
  {
    (yyval.args) = new AssignmentList();
  }
#line 2167 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 106: /* parameter_list: parameter  */
#line 624 "src/core/parser.y"
  {
    (yyval.args) = new AssignmentList();
    (yyval.args)->emplace_back((yyvsp[0].arg));
  }
#line 2176 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 107: /* parameter_list: parameter_list ',' parameter  */
#line 629 "src/core/parser.y"
  {
    (yyval.args) = (yyvsp[-2].args);
    (yyval.args)->emplace_back((yyvsp[0].arg));
  }
#line 2185 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 108: /* parameter: TOK_ID  */
#line 637 "src/core/parser.y"
  {
    (yyval.arg) = new Assignment((yyvsp[0].text), LOCD("assignment", (yyloc)));
    free((yyvsp[0].text));
  }
#line 2194 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 109: /* parameter: TOK_ID '=' expr  */
#line 642 "src/core/parser.y"
  {
    (yyval.arg) = new Assignment((yyvsp[-2].text), std::shared_ptr<Expression>((yyvsp[0].expr)),
                                 LOCD("assignment", (yyloc)));
    free((yyvsp[-2].text));
  }
#line 2203 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 110: /* arguments: %empty  */
#line 650 "src/core/parser.y"
  {
    (yyval.args) = new AssignmentList();
  }
#line 2211 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 112: /* argument_list: argument  */
#line 658 "src/core/parser.y"
  {
    (yyval.args) = new AssignmentList();
    (yyval.args)->emplace_back((yyvsp[0].arg));
  }
#line 2220 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 113: /* argument_list: argument_list ',' argument  */
#line 663 "src/core/parser.y"
  {
    (yyval.args) = (yyvsp[-2].args);
    (yyval.args)->emplace_back((yyvsp[0].arg));
  }
#line 2229 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 114: /* argument: expr  */
#line 671 "src/core/parser.y"
  {
    (yyval.arg) =
      new Assignment("", std::shared_ptr<Expression>((yyvsp[0].expr)), LOCD("argumentcall", (yyloc)));
  }
#line 2237 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

  case 115: /* argument: TOK_ID '=' expr  */
#line 675 "src/core/parser.y"
  {
    (yyval.arg) = new Assignment((yyvsp[-2].text), std::shared_ptr<Expression>((yyvsp[0].expr)),
                                 LOCD("argumentcall", (yyloc)));
    free((yyvsp[-2].text));
  }
#line 2246 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"
  break;

#line 2250 "/home/gsohler/git/pythonscad/build/objects/parser.cxx"

  default: break;
  }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT("-> $$ =", YY_CAST(yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK(yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp ? yytable[yyi] : yydefgoto[yylhs]);
  }

  goto yynewstate;

/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE(yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus) {
    ++yynerrs;
    yyerror(YY_("syntax error"));
  }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3) {
    /* If just tried and failed to reuse lookahead token after an
       error, discard it.  */

    if (yychar <= YYEOF) {
      /* Return failure if at end of input.  */
      if (yychar == YYEOF) YYABORT;
    } else {
      yydestruct("Error: discarding", yytoken, &yylval, &yylloc);
      yychar = YYEMPTY;
    }
  }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;

/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0) YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK(yylen);
  yylen = 0;
  YY_STACK_PRINT(yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;

/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3; /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;) {
    yyn = yypact[yystate];
    if (!yypact_value_is_default(yyn)) {
      yyn += YYSYMBOL_YYerror;
      if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror) {
        yyn = yytable[yyn];
        if (0 < yyn) break;
      }
    }

    /* Pop the current state because it cannot handle the error token.  */
    if (yyssp == yyss) YYABORT;

    yyerror_range[1] = *yylsp;
    yydestruct("Error: popping", YY_ACCESSING_SYMBOL(yystate), yyvsp, yylsp);
    YYPOPSTACK(1);
    yystate = *yyssp;
    YY_STACK_PRINT(yyss, yyssp);
  }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT(*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT("Shifting", YY_ACCESSING_SYMBOL(yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;

/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;

/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror(YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;

/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY) {
    /* Make sure we have latest lookahead translation.  See comments at
       user semantic actions for why this is necessary.  */
    yytoken = YYTRANSLATE(yychar);
    yydestruct("Cleanup: discarding lookahead", yytoken, &yylval, &yylloc);
  }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK(yylen);
  YY_STACK_PRINT(yyss, yyssp);
  while (yyssp != yyss) {
    yydestruct("Cleanup: popping", YY_ACCESSING_SYMBOL(+*yyssp), yyvsp, yylsp);
    YYPOPSTACK(1);
  }
#ifndef yyoverflow
  if (yyss != yyssa) YYSTACK_FREE(yyss);
#endif

  return yyresult;
}

#line 681 "src/core/parser.y"

int parserlex(void) { return lexerlex(); }

void yyerror(char const *s)
{
  // FIXME: We leak memory on parser errors...
  Location loc = Location(lexerget_lineno(), -1, -1, -1, sourcefile());
  LOG(message_group::Error, loc, "", "Parser error: %1$s", s);
}

#ifdef DEBUG
static Location debug_location(const std::string& info, const YYLTYPE& loc)
{
  auto location = LOC(loc);
  PRINTDB("%3d, %3d - %3d, %3d | %s",
          loc.first_line % loc.first_column % loc.last_line % loc.last_column % info);
  return location;
}
#endif

static void warn_reassignment(const Location& loc, const std::shared_ptr<Assignment>& assignment,
                              const fs::path& path)
{
  LOG(message_group::Warning, loc, path.parent_path().generic_string(),
      "%1$s was assigned on line %2$i but was overwritten", quoteVar(assignment->getName()),
      assignment->location().firstLine());
}

static void warn_reassignment(const Location& loc, const std::shared_ptr<Assignment>& assignment,
                              const fs::path& path1, const fs::path& path2)
{
  LOG(message_group::Warning, loc, path1.parent_path().generic_string(),
      "%1$s was assigned on line %2$i of %3$s but was overwritten", quoteVar(assignment->getName()),
      assignment->location().firstLine(), path2);
}

void handle_assignment(const std::string token, Expression *expr, const Location loc)
{
  bool found = false;
  for (auto& assignment : scope_stack.top()->assignments) {
    if (assignment->getName() == token) {
      auto mainFile = mainFilePath.string();
      auto prevFile = assignment->location().fileName();
      auto currFile = loc.fileName();

      const auto uncPathCurr = fs_uncomplete(currFile, mainFilePath.parent_path());
      const auto uncPathPrev = fs_uncomplete(prevFile, mainFilePath.parent_path());
      if (fileEnded) {
        // assignments via commandline
      } else if (prevFile == mainFile && currFile == mainFile) {
        // both assignments in the mainFile
        warn_reassignment(loc, assignment, mainFilePath);
      } else if (uncPathCurr == uncPathPrev) {
        // assignment overwritten within the same file
        // the line number being equal happens, when a file is included multiple times
        if (assignment->location().firstLine() != loc.firstLine()) {
          warn_reassignment(loc, assignment, mainFilePath, uncPathPrev);
        }
      } else if (prevFile == mainFile && currFile != mainFile) {
        // assignment from the mainFile overwritten by an include
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

bool parse(SourceFile *& file, const std::string& text, const std::string& filename,
           const std::string& mainFile, int debug)
{
  fs::path filepath;
  try {
    filepath = filename.empty() ? fs::current_path() : fs::absolute(fs::path{filename});
    mainFilePath = mainFile.empty() ? fs::current_path() : fs::absolute(fs::path{mainFile});
  } catch (const std::filesystem::filesystem_error& fs_err) {
    LOG(message_group::Error, "Parser error: file system error: %1$s", fs_err.what());
    return false;
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

  rootfile =
    new SourceFile(parser_sourcefile.parent_path().string(), parser_sourcefile.filename().string());
  scope_stack.push(&rootfile->scope);
  //        PRINTB_NOCACHE("New module: %s %p", "root" % rootfile);

  parserdebug = debug;
  int parserretval = -1;
  try {
    parserretval = parserparse();
  } catch (const HardWarningException& e) {
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

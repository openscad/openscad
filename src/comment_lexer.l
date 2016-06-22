%{
    #include "Assignment.h"
    #include "expression.h"
    #include "value.h"  
    #include "comment_parser.h"
    #include <boost/lexical_cast.hpp>
    YY_BUFFER_STATE yy_scan_string ( const char *str ) ;
%}


%x cond_string
D [0-9]
E [Ee][-+]?{D}+
H [0-9a-fA-F]

%%


{D}+{E}? |
{D}*\.{D}+{E}? |
{D}+\.{D}*{E}? {
                    try {
                        yylval.num = boost::lexical_cast<double>(yytext);
                        return NUM;
                    } catch (boost::bad_lexical_cast) {}
                }
                
"[" { return yytext[0];}
"]" { return yytext[0];}
"," { return yytext[0];}
":" { return yytext[0];}
   
[^(\[ \] \, \:)]* { yylval.text=strdup(yytext); return WORD;}

[\n\r ]

. { }

%%

int yywrap(void) {
return 1;
}




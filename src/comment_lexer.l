%{
    #include "typedefs.h"
    #include "expression.h"
    #include "value.h"  
    #include "comment_parser.h"
    #include<iostream>
    #include <boost/lexical_cast.hpp>
    using namespace std;
    YY_BUFFER_STATE yy_scan_string ( const char *str ) ;
%}


%x cond_string
D [0-9]
E [Ee][-+]?{D}+
H [0-9a-fA-F]

U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]
UNICODE {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

%%


{D}+{E}? |
{D}*\.{D}+{E}? |
{D}+\.{D}*{E}?          {
                            try {
                                yylval.num = boost::lexical_cast<double>(yytext);
                                return NUM;
                            } catch (boost::bad_lexical_cast) {}
                        }
                        
"=" {   yylval.ch=yytext[0]; return yytext[0];}
"[" {   yylval.ch=yytext[0]; return yytext[0];}
"]" {   yylval.ch=yytext[0]; return yytext[0];}
"," {   yylval.ch=yytext[0]; return yytext[0];}
":" {   yylval.ch=yytext[0]; return yytext[0];}
   
[^(/ \n \[ \] \, \: )]* { yylval.text=strdup(yytext); return WORD;}

[\n\r ]


. {   yylval.ch=yytext[0]; return yytext[0]; }
%%

int yywrap(void) {
return 1;
}




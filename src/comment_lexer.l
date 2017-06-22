%option prefix="comment_lexer"

%{
    #include "Assignment.h"
    #include "expression.h"
    #include "value.h"  
    #include "comment_parser.hxx"
    #include <boost/lexical_cast.hpp>
    YY_BUFFER_STATE yy_scan_string ( const char *str ) ;
    std::string stringcon;
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


[+-]?{D}+{E}? |
[+-]?{D}*\.{D}+{E}? |
[+-]?{D}+\.{D}*{E}? {
                    try {
                        comment_parserlval.num = boost::lexical_cast<double>(yytext);
                        return NUM;
                    } catch (boost::bad_lexical_cast) {}
                }
                
"[" { return yytext[0];}
"]" { return yytext[0];}
"," { return yytext[0];}
":" { return yytext[0];}
  
[ \t]

\"			{ BEGIN(cond_string); stringcon.clear(); }
<cond_string>{
\\n			{ stringcon += '\n'; }
\\t			{ stringcon += '\t'; }
\\r			{ stringcon += '\r'; }
\\\\			{ stringcon += '\\'; }
\\\"			{ stringcon += '"'; }
{UNICODE}               { stringcon += yytext; }
[^\\\n\"]		{ stringcon += yytext; }
\" 			{ BEGIN(INITIAL);
			comment_parserlval.text = strdup(stringcon.c_str());
			return WORD; }
<<EOF>> {   BEGIN(INITIAL);
			comment_parserlval.text = strdup(stringcon.c_str());
			return WORD; }
}

   
[^(\[ \] \, \" \:)]* { comment_parserlval.text=strdup(yytext); return WORD;}

. { }

%%

int yywrap(void) {
return 1;
}




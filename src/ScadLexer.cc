#include <boost/algorithm/string.hpp>
#include <string>

#include "ScadLexer.h"

ScadLexer::ScadLexer(QObject *parent) : QsciLexerCPP(parent)
{
	// -> Style: Keyword (lexer.l)
	keywordSet[0] =
		"if else let for each module function true false undef "
		"include use assert";

	// -> Style: KeywordSet2 (func.cc)
	keywordSet[1] =
		"abs sign rands min max sin cos asin acos tan atan atan2 "
		"round ceil floor pow sqrt exp len log ln str chr ord concat "
		"is_undef is_list is_num is_bool is_string is_function "
		"lookup search version version_num norm cross parent_module "
		"dxf_dim dxf_cross";

	// -> used in comments only like /*! \cube */
	keywordSet[2] =
		"struct union enum fn var def typedef file namespace package "
		"interface param see return class brief";

	// -> Style: GlobalClass
	keywordSet[3] =
		"cube sphere cylinder polyhedron square circle polygon text "
		"minkowski hull resize child children echo union difference "
		"intersection linear_extrude rotate_extrude import group "
		"projection render surface scale rotate mirror translate "
		"multmatrix color offset intersection_for roof";

    setFoldComments(true);
    setFoldAtElse(true);
}

ScadLexer::~ScadLexer()
{
}

const char *ScadLexer::language() const
{
	return "SCAD";
}

void ScadLexer::setKeywords(int set, const std::string& keywords)
{
	if ((set < 1) || (set > 4)) {
		return;
	}

	std::string trimmedKeywords(keywords);
        boost::algorithm::trim(trimmedKeywords);
	if (trimmedKeywords.empty()) {
		return;
	}

	keywordSet[set - 1] = trimmedKeywords;
}

const char *ScadLexer::keywords(int set) const
{
	if ((set < 1) || (set > 4)) {
		return nullptr;
	}
	return keywordSet[set - 1].c_str();
}

/***************************************************************/
/***************************************************************/
/***************************************************************/

#define ENABLE_LEXERTL  1

#if ENABLE_LEXERTL

#include <Qsci/qscilexercustom.h>
#include <Qsci/qsciscintilla.h>

#include "lexertl/dot.hpp"
#include "lexertl/generator.hpp"
#include "lexertl/lookup.hpp"


#if DEBUG_LEXERTL
#include "lexertl/debug.hpp"
#include <fstream>
#include <iostream>
#endif


/// See original attempt at https://github.com/openscad/openscad/tree/lexertl/src

Lex::Lex()
{
}

void Lex::rules(){

/*
for keywords, see
    void Builtins::initialize()
    
Questionable:
    assign
    class
    see
    brief
    undef
    namespace
    package
    interface
    param
    file
    typedef

TODO - need a double quoted string type
 */
	int s_size = sizeof(std::string);
	std::string keywords[] = {"var", "module", "function", "use", "echo", "include", "import", "group",
                            "projection", "render", "surface", "def", "enum", "struct", "fn", "typedef",
                            "file", "namespace", "package", "interface", "param", "see", "return", "class",
                            "brief", "if", "else", "let", "for", "undef"};
	int keywords_count = (sizeof(keywords)/s_size);

	std::string transformations[] = {"translate", "rotate", "child", "scale", "linear_extrude",
                                    "rotate_extrude", "resize", "mirror", "multmatrix", "color",
                                    "offset", "hull", "minkowski", "children", "assign", "intersection_for"};
	int transformations_count = (sizeof(transformations)/s_size);

	 std::string booleans[] = {"union", "difference", "intersection", "true", "false"};
	int booleans_count = (sizeof(booleans)/s_size);

	std::string functions[] = {"abs", "sign", "rands", "min", "max", "sin", "cos", "asin", "acos", "tan",
                                "atan", "atan2", "round", "ceil", "floor", "pow", "sqrt", "exp", "len",
                                "log", "ln", "str", "chr", "concat", "lookup", "search", "version",
                                "version_num", "norm", "cross", "parent_module", "dxf_dim", "dxf_cross"};
	int functions_count = (sizeof(functions)/s_size);
	 
	std::string models[] = {"sphere", "cube", "cylinder", "polyhedron", "square", "polygon", "text", "circle"};
	int models_count = (sizeof(models)/s_size);

	std::string operators[] = {"<=", ">=", "==", "!=", "&&", "="};
	int operators_count = (sizeof(operators)/s_size);


	rules_.push_state("COMMENT");
	defineRules(keywords, keywords_count, ekeyword);
	defineRules(transformations, transformations_count, etransformation);
	defineRules(booleans, booleans_count, eboolean);
	defineRules(functions, functions_count, efunction);
	defineRules(models, models_count, emodel);
	defineRules(operators, operators_count, eoperator);
 
	rules_.push("[0-9]+", enumber);
	rules_.push("[a-zA-Z0-9_]+", evariable);
	rules_.push("[$][a-zA-Z0-9_]+", especialVariable);

	rules_.push("INITIAL", "\"/*\"",  ecomment, "COMMENT");
	rules_.push("COMMENT", "[^*]+|.", ecomment,  "COMMENT");
	rules_.push("COMMENT", "\"*/\"", ecomment , "INITIAL");

	rules_.push("[/][/].*$", ecomment);
	rules_.push(".|\n", etext);
	lexertl::generator::build(rules_, sm);

#if DEBUG_LEXERTL
std::ofstream fout("file1.txt", std::fstream::trunc);
lexertl::debug::dump(sm, fout);
#endif

}

void Lex::defineRules(std::string words[], int size, int id){

	for(int it = 0; it < size; it++) {
		rules_.push(words[it], id);
	}
}

void Lex::lex_results(const std::string& input, int start, LexInterface* const obj){

	std::cout << "called lexer" <<std::endl;
	lexertl::smatch results (input.begin(), input.end());

	int isstyle = obj->getStyleAt(start-1);
	if(isstyle == 10)
		 results.state = 1;
	lexertl::lookup(sm, results);

	while(results.id != eEOF) {
		obj->highlighting(start, input, results);
		lexertl::lookup(sm, results);
	}
}

/***************************************************************/

ScadLexer2::ScadLexer2(QObject *parent) : QsciLexerCustom(parent), LexInterface()
{
	l = new Lex();
	l->rules();
}

ScadLexer2::~ScadLexer2()
{
	delete l;
}

void ScadLexer2::styleText(int start, int end)
{
std::cout<< "start: "<<start<<std::endl;
    if(!editor())
        return;

    char * data = new char[end - start + 1];
    editor()->SendScintilla(QsciScintilla::SCI_GETTEXTRANGE, start, end, data);
    QString source(data);
    const std::string input(source.toStdString());
    pos = editor()->SendScintilla(QsciScintilla::SCI_GETCURRENTPOS);

#if DEBUG_LEXERTL
std::cout << "its being called" <<std::endl;
#endif

    l->lex_results(input, start, this);
    this->fold(start, end);

    delete [] data;
    if(source.isEmpty())
        return;
}

void ScadLexer2::autoScroll(int error_pos)
{
    editor()->SendScintilla(QsciScintilla::SCI_GOTOPOS, error_pos);
    editor()->SendScintilla(QsciScintilla::SCI_SCROLLCARET);
}

void ScadLexer2::fold(int start, int end)
{
    char chNext = editor()->SendScintilla(QsciScintilla::SCI_GETCHARAT, start);
    int lineCurrent = editor()->SendScintilla(QsciScintilla::SCI_LINEFROMPOSITION, start);
    int levelPrev = editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent) & QsciScintilla::SC_FOLDLEVELNUMBERMASK;
    int levelCurrent = levelPrev;
    char ch;
    bool atEOL;
    bool style, startstyle;
    for (int i = start; i < end; i++) {
        ch = chNext;
        chNext = editor()->SendScintilla(QsciScintilla::SCI_GETCHARAT, i+1);
        
        atEOL = ((ch == '\r' && chNext != '\n') || (ch == '\n'));

        style = (editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, i-1) == 10);
        startstyle = (editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, i) == 10);

        if ((ch == '{') || (ch == '[') || ((!style) && (startstyle))) {
            levelCurrent++;
        } else 	if ((ch == '}') || (ch == ']') || ((style) && (!startstyle))) {
            levelCurrent--;
        }
            
        if (atEOL || (i == (end-1))) {
            int lev = levelPrev;
        
            if (levelCurrent > levelPrev) {
              lev |= QsciScintilla::SC_FOLDLEVELHEADERFLAG;
            }

            if ( lev != editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent)) {
              editor()->SendScintilla(QsciScintilla::SCI_SETFOLDLEVEL, lineCurrent , lev );
            }

            lineCurrent++;
            levelPrev = levelCurrent ;
        }
     }
    
    int flagsNext = editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent) & QsciScintilla::SC_FOLDLEVELNUMBERMASK;
	editor()->SendScintilla(QsciScintilla::SCI_SETFOLDLEVEL, lineCurrent, levelPrev | flagsNext);
}

const char *ScadLexer2::blockStart(int *style) const
{
    if (style)
        *style = 11;
    return "{ [";
}

const char *ScadLexer2::blockEnd(int *style) const
{
    if (style)
        *style = 11;
    return "}";
}

int ScadLexer2::getStyleAt(int pos)
{
	int sstyle = editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, pos);
	return sstyle;
}

void ScadLexer2::highlighting(int start, const std::string& input, lexertl::smatch results)
{
	std::string token = results.str();

#if DEBUG_LEXERTL
std::cout << "highlighting:" <<token<<std::endl;
#endif

	int style = results.id;
	QString word = QString::fromStdString(token);
	startStyling(start + std::distance(input.begin(), results.first));      // fishy, was results.start
	setStyling(word.length(), style);
}

QColor ScadLexer2::defaultColor(int style) const
{
    switch(style) {
        case Keyword:
            return Qt::blue;
        case Comment:
            return Qt::red;
        case Number:
            return Qt::green;
        case Transformation:
            return "#f32222";
        case Boolean:
            return "#22f322";
        case Function:
            return "#2222f3";
        case Model:
            return Qt::red;
        case Default:
            return Qt::black;
    }
    return Qt::black;
}

QString ScadLexer2::description(int style) const
{
    switch(style) {
        case Default:
            return "Default";
        case Keyword:
            return "Keyword";
        case Transformation:
            return "Transformation";
        case Boolean:
            return "Boolean";
        case Function:
            return "Function";
        case Operator:
            return "Operator";
        case Number:
            return tr("Number");
        case Variable:
            return "Variable";
        case SpecialVariable:
            return "SpecialVariable";
        case Comment:
            return "Comment";
    }
   return QString(style);
}

const char *ScadLexer2::language() const
{
	return "SCAD";
}

#endif

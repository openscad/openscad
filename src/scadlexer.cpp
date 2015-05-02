#include <boost/algorithm/string.hpp>
#include <Qsci/qsciscintilla.h>
#include "scadlexer.h"
#include <iostream>
ScadLexer::ScadLexer(QObject *parent) : QsciLexerCustom(parent)
{
	
    	keywordsList << "var" << "module" << "function" << "use"  <<
                    "include";

	transformationsList << "translate" << "rotate" << "scale" << "resize" << "mirror" << "multmatrix" << "color" << "offset" << "hull" <<"minkowski";

	booleansList << "union" << "difference" << "intersection" <<"true"<<"false";

	functionsList << "abs" << "sign" << "rands" << "min" << "max" << "sin" << "cos" <<  "asin" << "acos" << "tan" << "atan" << "atan2" <<"round" << "ceil" << "floor" << "pow" << "sqrt" << "exp" << "len" << "log" << "ln" << "str" << "chr" << "concat" << "lookup" << "search" << "version" << "version_num" << "norm" << "cross parent_module";

	modelsList << "sphere" << "cube" << "cylinder" << "polyhedron" << "square" << "polygon" << "text" << "circle";
		"dxf_dim dxf_cross";
	// -> Style: Keyword (lexer.l)
	keywordSet[0] =
		"if else let for module function true false undef "
		"include use";

	// -> Style: KeywordSet2 (func.cc)
	keywordSet[1] =
		"abs sign rands min max sin cos asin acos tan atan atan2 "
		"round ceil floor pow sqrt exp len log ln str chr concat "
		"lookup search version version_num norm cross parent_module "
		"dxf_dim dxf_cross";

	// -> used in comments only like /*! \cube */
	keywordSet[2] =
		"struct union enum fn var def typedef file namespace package "
		"interface param see return class brief";

	// -> Style: GlobalClass
	keywordSet[3] =
		"cube sphere cylinder polyhedron square circle polygon text "
		"minkowski hull resize child echo union difference "
		"intersection linear_extrude rotate_extrude import group  "
		"projection render surface scale rotate mirror translate "
		"multmatrix color offset ";
}

ScadLexer::~ScadLexer()
{
}

void ScadLexer::styleText(int start, int end)
{
    if(!editor())
        return;

    char * data = new char[end - start + 1];

    editor()->SendScintilla(QsciScintilla::SCI_GETTEXTRANGE, start, end, data);
    QString source(data);
    delete [] data;
    if(source.isEmpty())
        return;	

    highlightKeywords(source, start, keywordsList, Keyword);
    highlightKeywords(source, start, transformationsList, Transformation);
    highlightKeywords(source, start, booleansList, Boolean);
    highlightKeywords(source, start, functionsList, Function);
    highlightKeywords(source, start, modelsList, Model);
    highlightComments(source, start, Comment);
}

QColor ScadLexer::defaultColor(int style) const
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
	    return "22f322";
	case Function:
	    return "2222f3";
	case Model:
	    return Qt::red;
    }
    return Qt::black;
}

QString ScadLexer::description(int style) const
{
    switch(style) {
        case Default:
            return "Default";
        case Keyword:
            return "Keyword";
	case Comment:
	    return "Comment";
	case KeywordSet2:
	    return "KeywordSet2";
	case Number:
	    return tr("Number");
	case Transformation:
	    return "Transformation";
	case Boolean:
	    return "Boolean";
	case Function:
	    return "Function";
	case Model:
	    return "Model";
    }
   return QString(style);
}


void ScadLexer::highlightKeywords(const QString &source, int start, QStringList &List, int style )
{

    foreach(QString word, List) {
        if(source.contains(word)) {
            int p = source.count(word); 
		int index = 0; 
		while(p != 0) {
		
                int begin = source.indexOf(word, index); 
                index = begin+1; 
                startStyling(start + begin); 

                setStyling(word.length(), style);
                startStyling(start + begin); 
                p--;	
            }
        }
    }
}

void ScadLexer::highlightComments(const QString &source, int start, int style)
{

    int p = source.count("//"); 
    if(p == 0)
        return;
    int index = 0; 
    while(p != 0) {
        int begin = source.indexOf("//", index); 
        int length=0; 
        index = begin+1; 
	if(source.contains('\n')) {
		int endline = source.indexOf('\n', index);
		if (begin < endline){
			for(int k = begin; source[k] != '\n'; k++)
			    length++;
		}
	} else {
        for(int k = begin; source[k] != '\0'; k++) 
            length++;
	}
        startStyling(start + begin); 
        setStyling(length, Comment); 
        startStyling(start + begin); 

        p--;
    }
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
		return 0;
	}
	return keywordSet[set - 1].c_str();
}



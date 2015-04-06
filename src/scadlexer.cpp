#include <boost/algorithm/string.hpp>
#include <Qsci/qsciscintilla.h>
#include "scadlexer.h"

ScadLexer::ScadLexer(QObject *parent) : QsciLexerCustom(parent)
{
	
    	keywordsList << "var" << "module" << "function" << "use"  <<
                    "include";

	transformationsList << "translate" << "rotate" << "scale" << "resize" << "mirror" << "multmatrix" << "color" << "offset" << "hull" <<"minkowski";

	booleansList << "union" << "difference" << "intersection";

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

//	paintComments(source, start);
    highlightKeywords(source, start);
    highlightTransformations(source, start);
    highlightBooleans(source, start);
    highlightFunctions(source, start);
    highlightModels(source, start);
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


void ScadLexer::highlightKeywords(const QString &source, int start)
{
    foreach(QString word, keywordsList) { 
        if(source.contains(word)) {
            int p = source.count(word); 
            int index = 0; 
            while(p != 0) {
                int begin = source.indexOf(word, index); 
                index = begin+1; 

                startStyling(start + begin); 
                setStyling(word.length(), Keyword); 
                startStyling(start + begin); 

                p--;
            }
        }
    }
}
void ScadLexer::highlightTransformations(const QString &source, int start)
{
    foreach(QString word, transformationsList) { 
        if(source.contains(word)) {
            int p = source.count(word); 
            int index = 0; 
            while(p != 0) {
                int begin = source.indexOf(word, index); 
                index = begin+1; 

                startStyling(start + begin); 
                setStyling(word.length(), Transformation); 
                startStyling(start + begin); 

                p--;
            }
        }
    }
}
void ScadLexer::highlightBooleans(const QString &source, int start)
{
    foreach(QString word, booleansList) { 
        if(source.contains(word)) {
            int p = source.count(word); 
            int index = 0; 
            while(p != 0) {
                int begin = source.indexOf(word, index); 
                index = begin+1; 

                startStyling(start + begin); 
                setStyling(word.length(), Boolean); 
                startStyling(start + begin); 

                p--;
            }
        }
    }
}
void ScadLexer::highlightFunctions(const QString &source, int start)
{
    foreach(QString word, functionsList) { 
        if(source.contains(word)) {
            int p = source.count(word); 
            int index = 0; 
            while(p != 0) {
                int begin = source.indexOf(word, index); 
                index = begin+1; 

                startStyling(start + begin); 
                setStyling(word.length(), Function); 
                startStyling(start + begin); 

                p--;
            }
        }
    }
}
void ScadLexer::highlightModels(const QString &source, int start)
{
    foreach(QString word, modelsList) { 
        if(source.contains(word)) {
            int p = source.count(word); 
            int index = 0; 
            while(p != 0) {
                int begin = source.indexOf(word, index); 
                index = begin+1; 

                startStyling(start + begin); 
                setStyling(word.length(), Model); 
                startStyling(start + begin); 

                p--;
            }
        }
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


QStringList ScadLexer::autoCompletionWordSeparators() const
{
	QStringList wl;
	wl << "::" << "->" << ".";

	return wl;
}

const char *ScadLexer::blockStartKeyword(int *style) const
{
	return 0;
}

const char *ScadLexer::blockStart(int *style) const
{
	return 0;
}

const char *ScadLexer::blockEnd(int *style) const
{
	return 0;
}

int ScadLexer::braceStyle() const
{
	return 0;
}

const char *ScadLexer::wordCharacters() const
{

	return 0;
}

bool ScadLexer::defaultEolFill(int style) const
{
	return 0;
}

QFont ScadLexer::defaultFont(int style) const
{
	return QFont("Courier New",10);
}

QColor ScadLexer::defaultPaper(int style) const
{
	return Qt::blue;
}

void ScadLexer::refreshProperties()
{
}

bool ScadLexer::readProperties(QSettings &qs, const QString &prefix)
{
	return 0;
}

bool ScadLexer::writeProperties(QSettings &qs, const QString &prefix) const
{
	return 0;

}

void ScadLexer::setFoldAtElse(bool fold)
{
}

void ScadLexer::setAtElseProp()
{
}

void ScadLexer::setFoldComments(bool fold)
{
}

void ScadLexer::setCommentProp()
{
}

void ScadLexer::setFoldCompact(bool fold)
{
}

void ScadLexer::setCompactProp()
{
}

void ScadLexer::setFoldPreprocessor(bool fold)
{
}

void ScadLexer::setPreprocProp()
{
}

void ScadLexer::setStylePreprocessor(bool style)
{
}

void ScadLexer::setStylePreprocProp()
{
}

void ScadLexer::setDollarsAllowed(bool allowed)
{
}

void ScadLexer::setDollarsProp()
{
}

void ScadLexer::setHighlightTripleQuotedStrings(bool enabled)
{
}

void ScadLexer::setHighlightHashProp()
{
}

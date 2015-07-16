#include "scadlexer.h"

ScadLexer::ScadLexer(QObject *parent) : QsciLexerCustom(parent), LexInterface()
{
	l = new Lex();
	l->rules();

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
	delete l;
}

void ScadLexer::styleText(int start, int end)
{
	
    if(!editor())
        return;
    char * data = new char[end - start + 1];
    editor()->SendScintilla(QsciScintilla::SCI_GETTEXTRANGE, start, end, data);
    QString source(data);
	const std::string input(source.toStdString());
    	pos = editor()->SendScintilla(QsciScintilla::SCI_GETCURRENTPOS);
	int startStyle = editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, start);
	int posStyle = editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, pos);
	char ch = editor()->SendScintilla(QsciScintilla::SCI_GETCHARAT, pos);
	std::cout<<"style: "<<posStyle<<"token: "<< ch<<std::endl;
	l->lex_results(input, start, this, startStyle, posStyle);

    delete [] data;
    if(source.isEmpty())
        return;
}

void ScadLexer::highlighting(int start, const std::string& input, lexertl::smatch results, int style)
{
//	std::cout << results.id <<std::endl;
	QString word = QString::fromStdString(l->token);
	startStyling(start + std::distance(input.begin(), results.start));
	setStyling(word.length(), style);
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

QString ScadLexer::description(int style) const
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

#include <iostream>
#include "scadlexer.h"

ScadLexer::ScadLexer(QObject *parent) : QsciLexerCustom(parent)
{

    	keywordsList << "var" << "module" << "function" << "use"  << "echo" <<
                    "include" << "import" << "group" << "projection" << "render" << "surface" <<
		    "def" << "enum" <<"struct" << "def"<< "fn" << "typedef" << "file" << "namespace" << "package" <<
		    "interface" << "param" << "see" << "return" << "class" << "brief" <<
		    "if" << "else" << "let" << "for" << "undef";

	transformationsList << "translate" << "rotate" << "child" << "scale" << "linear_extrude" << "rotate_extrude" << "resize" << "mirror" << "multmatrix" << "color" << "offset" << "hull" <<"minkowski";

	booleansList << "union" << "difference" << "intersection" <<"true"<<"false";

	functionsList << "abs" << "sign" << "rands" << "min" << "max" << "sin" << "cos" <<  "asin" << "acos" << "tan" << "atan" << "atan2" <<"round" << "ceil" << "floor" << "pow" << "sqrt" << "exp" << "len" << "log" << "ln" << "str" << "chr" << "concat" << "lookup" << "search" << "version" << "version_num" << "norm" << "cross" << "parent_module" << "dxf_dim" << "dxf_cross";

	modelsList << "sphere" << "cube" << "cylinder" << "polyhedron" << "square" << "polygon" << "text" << "circle";
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
	defineRules(keywordsList, 3);
	defineRules(transformationsList, 4);
	defineRules(modelsList, 5);
	defineRules(booleansList, 6);
	defineRules(functionsList, 7);
	
	rules_.push( "[0-9]+", 1);	
	rules_.push( "[a-z]+", 2);
	lexertl::generator::build(rules_, sm);


    char * data = new char[end - start + 1];

    editor()->SendScintilla(QsciScintilla::SCI_GETTEXTRANGE, start, end, data);
    QString source(data);
	const std::string input(source.toStdString());
	lexertl::smatch results (input.begin(), input.end());
	lexertl::lookup(sm, results);	
	while(results.id != 0)
	{
		switch(results.id)
		{
			case 3:
		 	 token = results.str();
		  	 highlighting(start, input, results, Keyword);
		 	 break;
		
			case 1:
		  	 token = results.str();
		  	 highlighting(start, input, results, Number);
			 break;
			
			case 4:
		  	 token = results.str();
		  	 highlighting(start, input, results, Transformation);
			 break;

			case 5:
		  	 token = results.str();
		  	 highlighting(start, input, results, Model);
			 break;
			
			case 6:
		  	 token = results.str();
		  	 highlighting(start, input, results, Boolean);
			 break;
			
	       }
		lexertl::lookup(sm, results);
	}
    delete [] data;
    if(source.isEmpty())
        return;
}

void ScadLexer::defineRules(QStringList &List, int id)
{
	foreach(QString tok_, List){
	 std::string token = tok_.toStdString();
	 rules_.push(token, id);
	}
}

void ScadLexer::highlighting(int start, const std::string& input, lexertl::smatch results, int style)
{	
	  QString word = QString::fromStdString(token);
	  startStyling(start + std::distance(input.begin(), results.start));
	  setStyling(word.length(), style); 
	  startStyling(start + std::distance(input.begin(), results.start));
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

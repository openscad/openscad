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
	int MARGIN_SCRIPT_FOLD_INDEX = 1;	
    if(!editor())
        return;
    char * data = new char[end - start + 1];
    editor()->SendScintilla(QsciScintilla::SCI_GETTEXTRANGE, start, end, data);
    QString source(data);
	const std::string input(source.toStdString());
    	pos = editor()->SendScintilla(QsciScintilla::SCI_GETCURRENTPOS);
	l->lex_results(input, start, this);
	this->fold(start, end);
    editor()->SendScintilla(QsciScintilla::SCI_SETMARGINTYPEN,  MARGIN_SCRIPT_FOLD_INDEX, QsciScintilla::SC_MARGIN_SYMBOL);
    //editor()->SendScintilla(QsciScintilla::SCI_SETMARGINMASKN, MARGIN_SCRIPT_FOLD_INDEX, QsciScintilla::SC_MASK_FOLDERS);
    editor()->SendScintilla(QsciScintilla::SCI_SETMARGINWIDTHN, MARGIN_SCRIPT_FOLD_INDEX, 20);
    editor()->SendScintilla(QsciScintilla::SCI_SETMARGINSENSITIVEN, MARGIN_SCRIPT_FOLD_INDEX, 1);

  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDER, QsciScintilla::SC_MARK_BOXPLUS);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDEROPEN, QsciScintilla::SC_MARK_BOXMINUS);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDEREND, QsciScintilla::SC_MARK_EMPTY);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDERMIDTAIL, QsciScintilla::SC_MARK_EMPTY);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDEROPENMID, QsciScintilla::SC_MARK_EMPTY);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDERSUB, QsciScintilla::SC_MARK_EMPTY);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDERTAIL, QsciScintilla::SC_MARK_EMPTY);
  //editor()->SendScintilla(QsciScintilla::SCI_SETFOLDFLAGS, 16, 0); // 16  	Draw line below if not expanded


    delete [] data;
    if(source.isEmpty())
        return;
}
void ScadLexer::fold(int start, int end)
{
    char chNext = editor()->SendScintilla(QsciScintilla::SCI_GETCHARAT, start);
    int lineCurrent = editor()->SendScintilla(QsciScintilla::SCI_LINEFROMPOSITION, start);
    int levelPrev = editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent) & QsciScintilla::SC_FOLDLEVELNUMBERMASK;
    int levelCurrent = levelPrev;
    char ch;
    bool atEOL;

    for (int i = start; i < end; i++)
    {
	ch = chNext;
	chNext = editor()->SendScintilla(QsciScintilla::SCI_GETCHARAT, i+1);

	atEOL = ((ch == '\r' && chNext != '\n') || (ch == '\n'));

	if (ch == '{') {
	std::cout << "open" <<std::endl;
		levelCurrent++;
	}
	if (ch == '}') {
	std::cout << "closed" <<std::endl;
		levelCurrent--;
	}
	if (atEOL || (i == (end-1))) {
		int lev = levelPrev;
	
		if (levelCurrent > levelPrev) {
		  lev |= QsciScintilla::SC_FOLDLEVELHEADERFLAG;
		}

		if ( lev != editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent)) {
		  editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent , lev );
			std::cout << "line: "<<lineCurrent<<" lev: "<<lev<<std::endl;
		}

		lineCurrent++;
		levelPrev = levelCurrent ;
	}
     }
}

int ScadLexer::getStyleAt(int pos)
{
	int sstyle = editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, pos);
	return sstyle;
}

void ScadLexer::highlighting(int start, const std::string& input, lexertl::smatch results)
{
	std::string token = results.str();
	int style = results.id;
	QString word = QString::fromStdString(token);
	startStyling(start + std::distance(input.begin(), results.start));
	setStyling(word.length(), style);
    
/*	editor()->SendScintilla(QsciScintilla::SCI_SETPROPERTY, "fold", "1");
    editor()->SendScintilla(QsciScintilla::SCI_SETPROPERTY, "fold.compact", "0");
    editor()->SendScintilla(QsciScintilla::SCI_SETPROPERTY, "fold.comment", "1");
    editor()->SendScintilla(QsciScintilla::SCI_SETPROPERTY, "fold.preprocessor", "1");
    editor()->SendScintilla(QsciScintilla::SCI_SETMARGINTYPEN,  MARGIN_SCRIPT_FOLD_INDEX, QsciScintilla::SC_MARGIN_SYMBOL);
    editor()->SendScintilla(QsciScintilla::SCI_SETMARGINMASKN, MARGIN_SCRIPT_FOLD_INDEX, QsciScintilla::SC_MASK_FOLDERS);
    editor()->SendScintilla(QsciScintilla::SCI_SETMARGINWIDTHN, MARGIN_SCRIPT_FOLD_INDEX, 20);
    editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDER, QsciScintilla::SC_MARK_PLUS);

  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDEROPEN, QsciScintilla::SC_MARK_MINUS);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDEREND, QsciScintilla::SC_MARK_EMPTY);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDERMIDTAIL, QsciScintilla::SC_MARK_EMPTY);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDEROPENMID, QsciScintilla::SC_MARK_EMPTY);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDERSUB, QsciScintilla::SC_MARK_EMPTY);
  editor()->SendScintilla(QsciScintilla::SCI_MARKERDEFINE, QsciScintilla::SC_MARKNUM_FOLDERTAIL, QsciScintilla::SC_MARK_EMPTY);
*/
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

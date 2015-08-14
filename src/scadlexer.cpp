#include "scadlexer.h"

ScadLexer::ScadLexer(QObject *parent) : QsciLexerCustom(parent), LexInterface()
{
	l = new Lex();
	l->rules();
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

    l->lex_results(input, start, this);
    this->fold(start, end);

    delete [] data;
    if(source.isEmpty())
        return;
}

void ScadLexer::autoScroll(int error_pos)
{
    editor()->SendScintilla(QsciScintilla::SCI_GOTOPOS, error_pos);
    editor()->SendScintilla(QsciScintilla::SCI_SCROLLCARET);
}
void ScadLexer::fold(int start, int end)
{
    char chNext = editor()->SendScintilla(QsciScintilla::SCI_GETCHARAT, start);
    int lineCurrent = editor()->SendScintilla(QsciScintilla::SCI_LINEFROMPOSITION, start);
    int levelPrev = editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, lineCurrent) & QsciScintilla::SC_FOLDLEVELNUMBERMASK;
    int levelCurrent = levelPrev;
    char ch;
    bool atEOL;
    bool style, startstyle;
    for (int i = start; i < end; i++)
    {
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


const char *ScadLexer::blockStart(int *style) const
{
    if (style)
        *style = 11;
    return "{ [";
}


const char *ScadLexer::blockEnd(int *style) const
{
    if (style)
        *style = 11;
    return "}";
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

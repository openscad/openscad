#include <QString>
#include <QChar>
#include "scintillaeditor.h"
#include "Preferences.h"

ScintillaEditor::ScintillaEditor(QWidget *parent) : EditorInterface(parent)
{
	scintillaLayout = new QVBoxLayout(this);
	qsci = new QsciScintilla(this);
	scintillaLayout->setContentsMargins(0, 0, 0, 0);
	scintillaLayout->addWidget(qsci);
	qsci->setBraceMatching (QsciScintilla::SloppyBraceMatch);
	qsci->setWrapMode(QsciScintilla::WrapWord);
	qsci->setWrapVisualFlags(QsciScintilla::WrapFlagByText, QsciScintilla::WrapFlagByText, 0);
	qsci->setAutoIndent(true);
	qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, indicatorNumber);
	qsci->markerDefine(QsciScintilla::Circle, markerNumber);
	preferenceEditorOption = Preferences::inst()->getValue("editor/syntaxhighlight").toString();
	lexer = new ScadLexer(this);
	
	if(preferenceEditorOption == "For Light Background")
		forLightBackground();
	if(preferenceEditorOption == "For Dark Background")
		forDarkBackground();
	
	qsci->setCaretLineVisible(true);
	initFont();
        initMargin();
        initLexer();
}
void ScintillaEditor::indentSelection()
{
	
}
void ScintillaEditor::unindentSelection()
{
	 
}
void ScintillaEditor::commentSelection() 
{

}
void ScintillaEditor::uncommentSelection()
{
	
}
void ScintillaEditor::setPlainText(const QString &text)
{
	qsci->setText(text); 
}

QString ScintillaEditor::toPlainText()
{
	return qsci->text();
}

void ScintillaEditor::highlightError(int error_pos) 
{
	int line, index;
	qsci->lineIndexFromPosition(error_pos, &line, &index);
	qsci->fillIndicatorRange(line, index, line, index+1, indicatorNumber);	
	qsci->setIndicatorForegroundColor(QColor(255,0,0,100));
	qsci->markerAdd(line, markerNumber);
}

void ScintillaEditor::unhighlightLastError() 
{
	int totalLength = qsci->text().length();
	int line, index;
	qsci->lineIndexFromPosition(totalLength, &line, &index);
	qsci->clearIndicatorRange(0, 0, line, index, indicatorNumber);
	qsci->markerDeleteAll(markerNumber);
}

void ScintillaEditor::forLightBackground()
{
	lexer->setPaper(Qt::white);
	lexer->setColor(QColor("#272822")); // -> Style: Default text
	lexer->setColor(Qt::red, QsciLexerCPP::Keyword);	    // -> Style: Keyword	
	lexer->setColor(Qt::green, QsciLexerCPP::KeywordSet2);	    // -> Style: KeywordSet2
	lexer->setColor(Qt::blue, QsciLexerCPP::CommentDocKeyword);	    // -> used in comments only like /*! \cube */
	lexer->setColor(Qt::blue, QsciLexerCPP::GlobalClass);	    // -> Style: GlobalClass

	qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
	qsci->setCaretLineBackgroundColor(QColor("#ffe4e4"));

}

void ScintillaEditor::forDarkBackground()
{
	lexer->setPaper(QColor("#272822"));
	lexer->setColor(QColor(Qt::white));	// -> Style: Default text
	lexer->setColor(QColor("#66d9ef"), QsciLexerCPP::Keyword);	// -> Style: Keyword	
	lexer->setColor(QColor("#f92672"),QsciLexerCPP::KeywordSet2);  // -> Style: KeywordSet2	
	lexer->setColor(Qt::blue, QsciLexerCPP::CommentDocKeyword);		// -> used in comments only like /*! \cube */
	lexer->setColor(QColor("#fd9715"), QsciLexerCPP::GlobalClass); // -> Style: GlobalClass
		
	qsci->setCaretLineBackgroundColor(QColor("#eee"));
}

void ScintillaEditor::setHighlightScheme(const QString &name)
{

	if(name == "For Light Background")
	{
		forLightBackground();
	}
	else if(name == "For Dark Background")
	{
		forDarkBackground();
	}
	else
	return;
}

void ScintillaEditor::insertPlainText(const QString &text)
{
	qsci->setText(text); 
}

void ScintillaEditor::undo()
{
	qsci->undo(); 
}

void ScintillaEditor::redo()
{
	qsci->redo(); 
}

void ScintillaEditor::cut()
{
	qsci->cut();
}

void ScintillaEditor::copy()
{
	qsci->copy(); 
}

void ScintillaEditor::paste()
{ 
	qsci->paste();
}

void ScintillaEditor::zoomIn()
{
	qsci->zoomIn(); 
}

void ScintillaEditor::zoomOut() 
{
	qsci->zoomOut(); 
}

void ScintillaEditor::initFont()
{
    QFont font("Courier", 12);
    font.setFixedPitch(true);
    qsci->setFont(font);
}

void ScintillaEditor::initMargin()
{
    QFontMetrics fontmetrics = QFontMetrics(qsci->font());
    qsci->setMarginsFont(qsci->font());
    qsci->setMarginWidth(0, fontmetrics.width(QString::number(qsci->lines())) + 6);
    qsci->setMarginLineNumbers(0, true);
    qsci->setMarginsBackgroundColor(QColor("#cccccc"));
 
    connect(qsci, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

void ScintillaEditor::onTextChanged()
{
    QFontMetrics fontmetrics = qsci->fontMetrics();
    qsci->setMarginWidth(0, fontmetrics.width(QString::number(qsci->lines())) + 6);
}

void ScintillaEditor::initLexer()
{
    lexer->setDefaultFont(qsci->font());
    qsci->setLexer(lexer);
}

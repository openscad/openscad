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
	qsci->setUtf8(true);
	preferenceEditorOption = Preferences::inst()->getValue("editor/syntaxhighlight").toString();
	lexer = new ScadLexer(this);
	initFont();
	initLexer();
	initMargin();
	qsci->setFolding(QsciScintilla::BoxedTreeFoldStyle, 4);
	qsci->setCaretLineVisible(true);
	this->setHighlightScheme(preferenceEditorOption);	
	
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

bool ScintillaEditor::isContentModified()
{
  return qsci->isModified();
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
	lexer->setPaper("#fff");
	lexer->setColor(QColor("#272822")); // -> Style: Default text
	lexer->setColor(QColor("Green"), QsciLexerCPP::Keyword);	    // -> Style: Keyword	
	lexer->setColor(QColor("Green"), QsciLexerCPP::KeywordSet2);	    // -> Style: KeywordSet2
	lexer->setColor(Qt::blue, QsciLexerCPP::CommentDocKeyword);	    // -> used in comments only like /*! \cube */
	lexer->setColor(QColor("DarkBlue"), QsciLexerCPP::GlobalClass);	    // -> Style: GlobalClass
	lexer->setColor(Qt::blue, QsciLexerCPP::Operator);
	lexer->setColor(Qt::darkMagenta, QsciLexerCPP::DoubleQuotedString);	
	lexer->setColor(Qt::darkCyan, QsciLexerCPP::Comment);
	lexer->setColor(Qt::darkCyan, QsciLexerCPP::CommentLine);
	lexer->setColor(QColor("DarkRed"), QsciLexerCPP::Number);
	qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
	qsci->setCaretLineBackgroundColor(QColor("#ffe4e4"));
	qsci->setMarginsBackgroundColor(QColor("#ccc"));
	qsci->setMarginsForegroundColor(QColor("#111"));
}

void ScintillaEditor::forDarkBackground()
{
	lexer->setPaper(QColor("#272822"));
	lexer->setColor(QColor(Qt::white));          
	lexer->setColor(QColor("#f12971"), QsciLexerCPP::Keyword);
	lexer->setColor(QColor("#56dbf0"),QsciLexerCPP::KeywordSet2);	
	lexer->setColor(QColor("#ccdf32"), QsciLexerCPP::CommentDocKeyword);
	lexer->setColor(QColor("#56d8f0"), QsciLexerCPP::GlobalClass); 
	lexer->setColor(QColor("#d8d8d8"), QsciLexerCPP::Operator);
	lexer->setColor(QColor("#e6db74"), QsciLexerCPP::DoubleQuotedString);	
	lexer->setColor(QColor("#e6db74"), QsciLexerCPP::CommentLine);
	lexer->setColor(QColor("#af7dff"), QsciLexerCPP::Number);
	qsci->setCaretLineBackgroundColor(QColor(104,225,104, 127));
	qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
	qsci->setMarginsBackgroundColor(QColor("20,20,20,150"));
	qsci->setMarginsForegroundColor(QColor("#fff"));

}

void ScintillaEditor::Monokai()
{
	lexer->setPaper("#272822");
	lexer->setColor(QColor("#f8f8f2")); // -> Style: Default text
	lexer->setColor(QColor("#66c3b3"), QsciLexerCPP::Keyword);	    // -> Style: Keyword	
	lexer->setColor(QColor("#79abff"), QsciLexerCPP::KeywordSet2);	    // -> Style: KeywordSet2
	lexer->setColor(QColor("#ccdf32"), QsciLexerCPP::CommentDocKeyword);	    // -> used in comments only like /*! \cube */
	lexer->setColor(QColor("#ffffff"), QsciLexerCPP::GlobalClass);	    // -> Style: GlobalClass
	lexer->setColor(QColor("#d8d8d8"), QsciLexerCPP::Operator);
	lexer->setColor(QColor("#e6db74"), QsciLexerCPP::DoubleQuotedString);	
	lexer->setColor(QColor("#75715e"), QsciLexerCPP::CommentLine);
	lexer->setColor(QColor("#7fb347"), QsciLexerCPP::Number);
	qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
	qsci->setCaretLineBackgroundColor(QColor("#3e3d32"));
	qsci->setMarginsBackgroundColor(QColor("#757575"));
	qsci->setMarginsForegroundColor(QColor("#f8f8f2"));
}

void ScintillaEditor::Solarized_light()
{
	lexer->setPaper("#fdf6e3");
	lexer->setColor(QColor("#657b83")); // -> Style: Default text
	lexer->setColor(QColor("#268ad1"), QsciLexerCPP::Keyword);	    // -> Style: Keyword	
	lexer->setColor(QColor("#6c71c4"), QsciLexerCPP::KeywordSet2);	    // -> Style: KeywordSet2
	lexer->setColor(QColor("#b58900"), QsciLexerCPP::CommentDocKeyword);	    // -> used in comments only like /*! \cube */
	lexer->setColor(QColor("#b58800"), QsciLexerCPP::GlobalClass);	    // -> Style: GlobalClass
	lexer->setColor(QColor("#859900"), QsciLexerCPP::Operator);
	lexer->setColor(QColor("#2aa198"), QsciLexerCPP::DoubleQuotedString);	
	lexer->setColor(QColor("#b58800"), QsciLexerCPP::CommentLine);
	lexer->setColor(QColor("#cb4b16"), QsciLexerCPP::Number);
	qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
	qsci->setCaretLineBackgroundColor(QColor("#eeead5"));
	qsci->setMarginsBackgroundColor(QColor("#eee8d5"));
	qsci->setMarginsForegroundColor(QColor("#93a1a1"));
}

void ScintillaEditor::noColor()
{
	lexer->setPaper(Qt::white);
	lexer->setColor(Qt::black);
	qsci->setMarginsBackgroundColor(QColor("#ccc"));
	qsci->setMarginsForegroundColor(QColor("#111"));
	
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
	else if(name == "Monokai")
	{
		Monokai();
	}
	else if(name == "Solarized")
	{
		Solarized_light();
	}
	else if(name == "Off")
	{
		noColor();
	}
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
    QFont font("inconsolata", 12);
    font.setFixedPitch(true);
    qsci->setFont(font);
}

void ScintillaEditor::initLexer()
{
    lexer->setFont(qsci->font());
    qsci->setLexer(lexer);
}
void ScintillaEditor::initMargin()
{
    QFontMetrics fontmetrics = QFontMetrics(qsci->font());
    qsci->setMarginsFont(qsci->font());
    qsci->setMarginWidth(0, fontmetrics.width(QString::number(qsci->lines())) + 6);
    qsci->setMarginLineNumbers(0, true);
 
    connect(qsci, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

void ScintillaEditor::onTextChanged()
{
    QFontMetrics fontmetrics = qsci->fontMetrics();
    qsci->setMarginWidth(0, fontmetrics.width(QString::number(qsci->lines())) + 6);
}

bool ScintillaEditor::find(const QString &expr, QTextDocument::FindFlags options)
{
	qsci->findFirst(expr, false, false, false, false, true, 1, 1, true, false);
}

bool ScintillaEditor::findNext(QTextDocument::FindFlags options, QString& newText)
{
	qsci->findNext();
	return 0;
}

void ScintillaEditor::replaceSelectedText(QString& newText)
{
	qsci->replaceSelectedText(newText);
}


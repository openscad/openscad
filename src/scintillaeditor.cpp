#include <iostream>
#include <QString>
#include <QChar>
#include <Qsci/qscilexercpp.h>
#include "scintillaeditor.h"
#include "parsersettings.h"

ScintillaEditor::ScintillaEditor(QWidget *parent) : EditorInterface(parent)
{
	scintillaLayout = new QVBoxLayout(this);
	qsci = new QsciScintilla(this);
	scintillaLayout->addWidget(qsci);
	qsci->setBraceMatching (QsciScintilla::SloppyBraceMatch);
	qsci->setWrapMode(QsciScintilla::WrapWord);
	qsci->setWrapVisualFlags(QsciScintilla::WrapFlagByText, QsciScintilla::WrapFlagByText, 0);
	qsci->setAutoIndent(true);
	initFont();
        initMargin();
        initLexer();

}
void ScintillaEditor::indentSelection()
{
	int line, index;
	qsci->getCursorPosition(&line, &index);
	qsci->indent(line);
	
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
	qsci->fillIndicatorRange(line, index, line, index+1, 1);
	qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, 1);
	qsci->setIndicatorForegroundColor(QColor(255,0,0,100));
}
void ScintillaEditor::unhighlightLastError() 
{
//presently not working :(
	int line, index;
	 qsci->lineIndexFromPosition(parser_error_pos, &line, &index);
	qsci->clearIndicatorRange(line, index, line, index+1, 1);
	qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, 1);
}
void ScintillaEditor::setHighlightScheme(const QString &name)
{

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
    QsciLexerCPP *lexer = new QsciLexerCPP();
    lexer->setDefaultFont(qsci->font());
    lexer->setFoldComments(true);
    qsci->setLexer(lexer);
}

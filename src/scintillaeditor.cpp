#include <iostream>
#include <QString>
#include <QChar>
#include "scintillaeditor.h"

ScintillaEditor::ScintillaEditor(QWidget *parent) : EditorInterface(parent)
{
	scintillaLayout = new QVBoxLayout(this);
	qsci = new QsciScintilla(this);
	scintillaLayout->addWidget(qsci);
	qsci->setMarginLineNumbers(1,true);
}
void ScintillaEditor::indentSelection()
{
	if(qsci->hasSelectedText())
	{
		QString txt = qsci->selectedText();
		txt.replace(QString(QChar(8233)), QString(QChar(8233)) + QString("\t"));
        	if (txt.endsWith(QString(QChar(8233)) + QString("\t")))
                	txt.chop(1);
        	txt = QString("\t") + txt;
		qsci->replaceSelectedText(txt);
	}
}
void ScintillaEditor::unindentSelection()
{
	 
}
void ScintillaEditor::commentSelection() 
{}
void ScintillaEditor::uncommentSelection()
{}
void ScintillaEditor::setPlainText(const QString &text)
{
	qsci->setText(text); 
}
QString ScintillaEditor::toPlainText()
{
	return qsci->text();
}
void ScintillaEditor::highlightError(int error_pos) 
{}
void ScintillaEditor::unhighlightLastError() 
{}
void ScintillaEditor::setHighlightScheme(const QString &name)
{ }
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


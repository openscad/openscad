#include <iostream>
#include <QTextCursor>
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
	int line,index;
	int currentLine = qsci->positionFromLineIndex(line,index); 	
	std::cout << "--------------------------------------"<<line<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";	
	qsci->setIndentation(currentLine, 5); 
}
void ScintillaEditor::unindentSelection()
{ }
void ScintillaEditor::commentSelection() 
{}
void ScintillaEditor::uncommentSelection()
{}
void ScintillaEditor::setPlainText(const QString &text)
{ }
void ScintillaEditor::highlightError(int error_pos) 
{}
void ScintillaEditor::unhighlightLastError() 
{}
void ScintillaEditor::setHighlightScheme(const QString &name)
{ }
void ScintillaEditor::insertPlainText(const QString &text)
{ }

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


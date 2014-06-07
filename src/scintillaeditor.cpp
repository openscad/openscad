#include "scintillaeditor.h"

ScintillaEditor::ScintillaEditor(QWidget *parent) : Editor(parent)
{
	layout = new QVBoxLayout(this);
	qsci = new QsciScintilla(this);;
	layout->addWidget(qsci);
	qsci->setMarginLineNumbers(10,true);
}

#include "scintillaeditor.h"

ScintillaEditor::ScintillaEditor(QWidget *parent) : Editor(parent)
{
	qsci = new QsciScintilla;
	qsci->setMarginLineNumbers(10,true);
}

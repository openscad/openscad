#include "editor.h"
#include "Preferences.h"

EditorInterface::EditorInterface(QWidget *parent) : QWidget(parent)
{
	// This needed to avoid QTextEdit accepting filename drops as we want
	// to handle these ourselves in MainWindow
	setAcceptDrops(false);
	this->highlighter = new Highlighter(this->document());
}
EditorInterface::~EditorInterface()
{
	delete highlighter;
}

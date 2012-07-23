#include "editor.h"
#include "Preferences.h"

#include <iostream>

#ifndef _QCODE_EDIT_
void Editor::indentSelection()
{
	QTextCursor cursor = textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)), QString(QChar(8233)) + QString("\t"));
	if (txt.endsWith(QString(QChar(8233)) + QString("\t")))
		txt.chop(1);
	txt = QString("\t") + txt;

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	setTextCursor(cursor);
}

void Editor::unindentSelection()
{
	QTextCursor cursor = textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("\t"), QString(QChar(8233)));
	if (txt.startsWith(QString("\t")))
		txt.remove(0, 1);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	setTextCursor(cursor);
}

void Editor::commentSelection()
{
	QTextCursor cursor = textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)), QString(QChar(8233)) + QString("//"));
	if (txt.endsWith(QString(QChar(8233)) + QString("//")))
		txt.chop(2);
	txt = QString("//") + txt;

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	setTextCursor(cursor);
}

void Editor::uncommentSelection()
{
	QTextCursor cursor = textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("//"), QString(QChar(8233)));
	if (txt.startsWith(QString("//")))
		txt.remove(0, 2);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	setTextCursor(cursor);
}

void Editor::zoomIn()
{
	// We have the fontsize in two places. one, in the in-memory window font
	// information that the user sees on the screen, and two, in the
	// settings which are persistent on disk. Here we make sure they are
	// in sync - we assume the fontsize from the in-memory window to be accurate,
	// and trust that there is code elsewhere in OpenSCAD that has initialized
	// it properly. We update the on-disk Settings with whatever is in the window.
	//
	// And of course we increment by one before we do all this.
	// See also QT's implementation of QEditor
	QSettings settings;
	QFont tmp_font = this->font() ;
	std::cout << "in  fontsize cur" << tmp_font.pointSize() << "\n";
	if ( font().pointSize() >= 1 )
		tmp_font.setPointSize( 1 + font().pointSize() );
	else
		tmp_font.setPointSize( 1 );
	std::cout << "in new fontsize cur" << tmp_font.pointSize() << "\n";
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->setFont( tmp_font );
}

void Editor::zoomOut()
{
	QSettings settings;
	QFont tmp_font = this->font();
	std::cout << "out fontsize cur" << tmp_font.pointSize() << "\n";
	if ( font().pointSize() >= 2 )
		tmp_font.setPointSize( -1 + font().pointSize() );
	else
		tmp_font.setPointSize( 1 );
	std::cout << "out new fontsize cur" << tmp_font.pointSize() << "\n";
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->setFont( tmp_font );
}

#endif

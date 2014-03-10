#include "editor.h"
#include "Preferences.h"

Editor::Editor(QWidget *parent) : QTextEdit(parent)
{
	setAcceptRichText(false);
	this->highlighter = new Highlighter(this->document());
}

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
	// See also QT's implementation in QEditor.cpp
	QSettings settings;
	QFont tmp_font = this->font() ;
	if ( font().pointSize() >= 1 )
		tmp_font.setPointSize( 1 + font().pointSize() );
	else
		tmp_font.setPointSize( 1 );
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->setFont( tmp_font );
}

void Editor::zoomOut()
{
	QSettings settings;
	QFont tmp_font = this->font();
	if ( font().pointSize() >= 2 )
		tmp_font.setPointSize( -1 + font().pointSize() );
	else
		tmp_font.setPointSize( 1 );
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->setFont( tmp_font );
}

void Editor::wheelEvent ( QWheelEvent * event )
{
	if (event->modifiers() == Qt::ControlModifier) {
		if (event->delta() > 0 )
			zoomIn();
		else if (event->delta() < 0 )
			zoomOut();
	} else {
		QTextEdit::wheelEvent( event );
	}
}

void Editor::setPlainText(const QString &text)
{
	int y = verticalScrollBar()->sliderPosition();
	// Save current cursor position
	QTextCursor cursor = textCursor();
	int n = cursor.position();
	QTextEdit::setPlainText(text);
	// Restore cursor position
	if (n < text.length()) {
		cursor.setPosition(n);
		setTextCursor(cursor);
		verticalScrollBar()->setSliderPosition(y);
	}
}

void Editor::highlightError(int error_pos)
{
	highlighter->highlightError( error_pos );
        QTextCursor cursor = this->textCursor();
        cursor.setPosition( error_pos );
        this->setTextCursor( cursor );
}

void Editor::unhighlightLastError()
{
	highlighter->unhighlightLastError();
}

void Editor::setHighlightScheme(const QString &name)
{
	highlighter->assignFormatsToTokens( name );
	highlighter->rehighlight(); // slow on large files
}

Editor::~Editor()
{
	delete highlighter;
}

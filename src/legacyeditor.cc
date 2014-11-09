#include "legacyeditor.h"
#include "Preferences.h"
#include "highlighter.h"

LegacyEditor::LegacyEditor(QWidget *parent) : EditorInterface(parent)
{
	QVBoxLayout *legacyeditorLayout = new QVBoxLayout(this);
	legacyeditorLayout->setContentsMargins(0, 0, 0, 0);
	this->textedit = new QTextEdit(this);
	legacyeditorLayout->addWidget(this->textedit);
	this->textedit->setAcceptRichText(false);
	// This needed to avoid the editor accepting filename drops as we want
	// to handle these ourselves in MainWindow
	this->textedit->setAcceptDrops(false);
	this->textedit->setWordWrapMode(QTextOption::WrapAnywhere);
	this->textedit->setTabStopWidth(30);

	this->highlighter = new Highlighter(this->textedit->document());

	connect(this->textedit, SIGNAL(textChanged()), this, SIGNAL(contentsChanged()));
	connect(this->textedit->document(), SIGNAL(modificationChanged(bool)), this, SIGNAL(modificationChanged(bool)));
}

void LegacyEditor::indentSelection()
{	
	QTextCursor cursor = textedit->textCursor();
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
	textedit->setTextCursor(cursor);
}

void LegacyEditor::unindentSelection()
{
	QTextCursor cursor = textedit->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("\t"), QString(QChar(8233)));
	if (txt.startsWith(QString("\t")))
		txt.remove(0, 1);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	textedit->setTextCursor(cursor);
}

void LegacyEditor::commentSelection()
{
	QTextCursor cursor = textedit->textCursor();
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
	textedit->setTextCursor(cursor);

}

void LegacyEditor::uncommentSelection()
{
	QTextCursor cursor = textedit->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("//"), QString(QChar(8233)));
	if (txt.startsWith(QString("//")))
		txt.remove(0, 2);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	textedit->setTextCursor(cursor);
}

void LegacyEditor::zoomIn()
{
	// See also QT's implementation in QLegacyEditor.cpp
	QSettings settings;
	QFont tmp_font = this->font() ;
	if (font().pointSize() >= 1)
		tmp_font.setPointSize(1 + font().pointSize());
	else
		tmp_font.setPointSize(1);
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->setFont(tmp_font);
}

void LegacyEditor::zoomOut()
{

	QSettings settings;
	QFont tmp_font = this->font();
	if (font().pointSize() >= 2)
		tmp_font.setPointSize(-1 + font().pointSize());
	else
		tmp_font.setPointSize(1);
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->setFont(tmp_font);

}

void LegacyEditor::setPlainText(const QString &text)
{

	int y = textedit->verticalScrollBar()->sliderPosition();
	// Save current cursor position
	QTextCursor cursor = textedit->textCursor();
	int n = cursor.position();
	textedit->setPlainText(text);
	// Restore cursor position
	if (n < text.length()) {
		cursor.setPosition(n);
		textedit->setTextCursor(cursor);
		textedit->verticalScrollBar()->setSliderPosition(y);
	}
}

void LegacyEditor::highlightError(int error_pos)
{
	highlighter->highlightError(error_pos);
        QTextCursor cursor = this->textedit->textCursor();
        cursor.setPosition(error_pos);
        this->textedit->setTextCursor(cursor);

}

void LegacyEditor::unhighlightLastError()
{
	highlighter->unhighlightLastError();
}

void LegacyEditor::setHighlightScheme(const QString &name)
{
	highlighter->assignFormatsToTokens(name);
	highlighter->rehighlight(); // slow on large files
}

QSize LegacyEditor::sizeHint() const
{
	if (initialSizeHint.width() <= 0) {
		return textedit->sizeHint();
	} else {
		return initialSizeHint;
	}
}

void LegacyEditor::setInitialSizeHint(const QSize &size)
{
	initialSizeHint = size;
}
 
QString LegacyEditor::toPlainText()
{
	return textedit->toPlainText();
}

void LegacyEditor::insert(const QString &text)
{
	textedit->insertPlainText(text);
}

void LegacyEditor::replaceAll(const QString &text)
{
	textedit->selectAll();
	textedit->insertPlainText(text);
}

void LegacyEditor::undo()
{
	textedit->undo();
}

void LegacyEditor::redo()
{
	textedit->redo();
}

void LegacyEditor::cut()
{
	textedit->cut();
}

void LegacyEditor::copy()
{
	textedit->copy();
}

void LegacyEditor::paste()
{
	textedit->paste();
}

LegacyEditor::~LegacyEditor()
{
	delete highlighter;
}

void LegacyEditor::replaceSelectedText(const QString &newText)
{
	QTextCursor cursor = this->textedit->textCursor();
	if (cursor.selectedText() != newText) {
		cursor.insertText(newText);
	}
}

bool LegacyEditor::findString(const QString & exp, bool findBackwards) const
{
	return textedit->find(exp, findBackwards ? QTextDocument::FindBackward : QTextDocument::FindFlags(0));
}

bool LegacyEditor::find(const QString &newText, bool findNext, bool findBackwards)
{
	bool success = this->findString(newText, findBackwards);
	if (!success) { // Implement wrap-around search behavior
		QTextCursor old_cursor = this->textedit->textCursor();
		QTextCursor tmp_cursor = old_cursor;
		tmp_cursor.movePosition(findBackwards ? QTextCursor::End : QTextCursor::Start);
		this->textedit->setTextCursor(tmp_cursor);
		bool success = this->findString(newText, findBackwards);
		if (!success) {
			this->textedit->setTextCursor(old_cursor);
		}
		return success;
	}
	return true;

}

void LegacyEditor::initFont(const QString& family, uint size)
{
	QFont font;
	if (!family.isEmpty()) font.setFamily(family);
	else font.setFixedPitch(true);
	if (size > 0)	font.setPointSize(size);
	font.setStyleHint(QFont::TypeWriter);
	this->setFont(font);

}

QString LegacyEditor::selectedText()
{
	return textedit->textCursor().selectedText();
}

void LegacyEditor::setContentModified(bool y)
{
	textedit->document()->setModified(y);
}

bool LegacyEditor::isContentModified()
{
	return textedit->document()->isModified();
}

QStringList LegacyEditor::colorSchemes()
{
	QStringList colorSchemes;
    
	colorSchemes
	    << "For Light Background"
	    << "For Dark Background"
	    << "Off";
	
	return colorSchemes;
}

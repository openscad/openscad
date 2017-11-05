#include "legacyeditor.h"
#include "Preferences.h"
#include "highlighter.h"
#include "QSettingsCached.h"

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
	QTextCursor cursor = this->textedit->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)), QString(QChar(8233)) + QString("\t"));
	if (txt.endsWith(QString(QChar(8233)) + QString("\t"))) txt.chop(1);
	txt = QString("\t") + txt;

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	this->textedit->setTextCursor(cursor);
}

void LegacyEditor::unindentSelection()
{
	QTextCursor cursor = this->textedit->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("\t"), QString(QChar(8233)));
	if (txt.startsWith(QString("\t"))) txt.remove(0, 1);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	this->textedit->setTextCursor(cursor);
}

void LegacyEditor::commentSelection()
{
	QTextCursor cursor = this->textedit->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)), QString(QChar(8233)) + QString("//"));
	if (txt.endsWith(QString(QChar(8233)) + QString("//"))) txt.chop(2);
	txt = QString("//") + txt;

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	this->textedit->setTextCursor(cursor);

}

void LegacyEditor::uncommentSelection()
{
	QTextCursor cursor = this->textedit->textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("//"), QString(QChar(8233)));
	if (txt.startsWith(QString("//"))) txt.remove(0, 2);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	this->textedit->setTextCursor(cursor);
}

void LegacyEditor::zoomIn()
{
	// See also QT's implementation in QLegacyEditor.cpp
	QSettingsCached settings;
	QFont tmp_font = this->textedit->font();
	if (font().pointSize() >= 1) tmp_font.setPointSize(1 + font().pointSize());
	else tmp_font.setPointSize(1);
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->textedit->setFont(tmp_font);
}

void LegacyEditor::zoomOut()
{

	QSettingsCached settings;
	QFont tmp_font = this->textedit->font();
	if (font().pointSize() >= 2) tmp_font.setPointSize(-1 + font().pointSize());
	else tmp_font.setPointSize(1);
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->textedit->setFont(tmp_font);

}

void LegacyEditor::setPlainText(const QString &text)
{

	int y = this->textedit->verticalScrollBar()->sliderPosition();
	// Save current cursor position
	QTextCursor cursor = this->textedit->textCursor();
	int n = cursor.position();
	this->textedit->setPlainText(text);
	// Restore cursor position
	if (n < text.length()) {
		cursor.setPosition(n);
		this->textedit->setTextCursor(cursor);
		this->textedit->verticalScrollBar()->setSliderPosition(y);
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
		return this->textedit->sizeHint();
	}
	else {
		return initialSizeHint;
	}
}

void LegacyEditor::setInitialSizeHint(const QSize &size)
{
	initialSizeHint = size;
}

QString LegacyEditor::toPlainText()
{
	return this->textedit->toPlainText();
}

void LegacyEditor::insert(const QString &text)
{
	this->textedit->insertPlainText(text);
}

void LegacyEditor::setText(const QString &text)
{
	this->textedit->selectAll();
	this->textedit->insertPlainText(text);
}

bool LegacyEditor::canUndo()
{
	return (this->textedit->document()->availableUndoSteps() != 0);
}

void LegacyEditor::undo()
{
	this->textedit->undo();
}

void LegacyEditor::redo()
{
	this->textedit->redo();
}

void LegacyEditor::cut()
{
	this->textedit->cut();
}

void LegacyEditor::copy()
{
	this->textedit->copy();
}

void LegacyEditor::paste()
{
	this->textedit->paste();
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

void LegacyEditor::replaceAll(const QString &findText, const QString &replaceText)
{
	QTextCursor cursor(this->textedit->textCursor());
	cursor.setPosition(0);
	this->textedit->setTextCursor(cursor);
	this->textedit->find(findText);
	while (this->textedit->textCursor().hasSelection()) {
		this->textedit->textCursor().insertText(replaceText);
		this->textedit->find(findText);
	}
}

bool LegacyEditor::findString(const QString &exp, bool findBackwards) const
{
	return this->textedit->find(exp, findBackwards ? QTextDocument::FindBackward : QTextDocument::FindFlags(0));
}

int LegacyEditor::resetFindIndicators(const QString & /*findText*/, bool /*visibility*/)//incomplete-place-holder
{
	int findwordcount = 0;
	// blank see scintillaeditor.cpp
	return findwordcount;
}

bool LegacyEditor::find(const QString &newText, bool /*findNext*/, bool findBackwards)
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

void LegacyEditor::initFont(const QString &family, uint size)
{
	QFont font;
	if (!family.isEmpty()) font.setFamily(family);
	else font.setFixedPitch(true);
	if (size > 0) font.setPointSize(size);
	font.setStyleHint(QFont::TypeWriter);
	this->textedit->setFont(font);

}

QString LegacyEditor::selectedText()
{
	return textedit->textCursor().selectedText();
}

void LegacyEditor::setContentModified(bool y)
{
	this->textedit->document()->setModified(y);
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

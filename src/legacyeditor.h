#pragma once

#include <QObject>
#include <QString>
#include <QWidget>
#include <QWheelEvent>
#include <QScrollBar>
#include <QTextEdit>
#include <QVBoxLayout>
#include "highlighter.h"
#include "editor.h"

class LegacyEditor : public EditorInterface
{
	Q_OBJECT
public:
	LegacyEditor(QWidget *parent);
	~LegacyEditor();
	QTextEdit *textedit;
	QSize sizeHint() const;
	void setInitialSizeHint(const QSize&);
	void setTabStopWidth(int);
	void wheelEvent (QWheelEvent*);
	QString	toPlainText();
	QTextCursor textCursor() const;
	void setTextCursor (const QTextCursor&);
	QTextDocument *document() { return textedit->document(); }
	bool find(const QString&, QTextDocument::FindFlags options = 0);
public slots:
	void zoomIn();
	void zoomOut();
	void setLineWrapping(bool on) { if(on) textedit->setWordWrapMode(QTextOption::WrapAnywhere); }
	void setContentModified(bool y) { textedit->document()->setModified(y);  }
	bool isContentModified() {return textedit->document()->isModified();}
	void indentSelection();
	void unindentSelection();
	void commentSelection();
	void uncommentSelection();
	void setPlainText(const QString&);
	void highlightError(int);
	void unhighlightLastError();
	void setHighlightScheme(const QString&);
	void insertPlainText(const QString&);
	void undo();
	void redo();
	void cut();
	void copy();
	void paste();
private:
	Highlighter *highlighter;
	QSize initialSizeHint;
	QVBoxLayout *legacyeditorLayout;
};

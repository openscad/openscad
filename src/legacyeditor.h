#pragma once

#include <QObject>
#include <QString>
#include <QWidget>
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
	virtual ~LegacyEditor();
	QSize sizeHint() const;
	void setInitialSizeHint(const QSize&);
	void setTabStopWidth(int);
	QString	toPlainText();
	QTextCursor textCursor() const;
	void setTextCursor (const QTextCursor&);
	QTextDocument *document() { return textedit->document(); }
	bool find(const QString &, bool findNext = false, bool findBackwards = false);
	void replaceSelectedText(const QString &newText);	
	bool findString(const QString & exp, bool findBackwards) const;

public slots:
	void zoomIn();
	void zoomOut();
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
	void insert(const QString&);
	void undo();
	void redo();
	void cut();
	void copy();
	void paste();
	void initFont(const QString&, uint);
private:
	QTextEdit *textedit;
	Highlighter *highlighter;
	QSize initialSizeHint;
	QVBoxLayout *legacyeditorLayout;
};

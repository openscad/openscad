#pragma once

#include "editor.h"

class LegacyEditor : public EditorInterface
{
	Q_OBJECT
public:
	LegacyEditor(class QWidget *parent);
	virtual ~LegacyEditor();
	QSize sizeHint() const;
	void setInitialSizeHint(const QSize&);
	QString	toPlainText();
	QString selectedText();
	bool find(const QString &, bool findNext = false, bool findBackwards = false);
	void replaceSelectedText(const QString &newText);	
	void replaceAll(const QString &findText, const QString &replaceText);
	bool findString(const QString & exp, bool findBackwards) const;
	QStringList colorSchemes();
    bool canUndo();

public slots:
	void zoomIn();
	void zoomOut();
	void setContentModified(bool);
	bool isContentModified();
	void indentSelection();
	void unindentSelection();
	void commentSelection();
	void uncommentSelection();
	void setPlainText(const QString&);
	void highlightError(int);
	void unhighlightLastError();
	void setHighlightScheme(const QString&);
	void insert(const QString&);
	void setText(const QString&);
	void undo();
	void redo();
	void cut();
	void copy();
	void paste();
	void initFont(const QString&, uint);
private:
	class QTextEdit *textedit;
	class Highlighter *highlighter;
	QSize initialSizeHint;
};

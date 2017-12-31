#pragma once

#include "editor.h"

class LegacyEditor : public EditorInterface
{
	Q_OBJECT
public:
	LegacyEditor(class QWidget *parent);
	~LegacyEditor();
	QSize sizeHint() const override;
	void setInitialSizeHint(const QSize&) override;
	QString	toPlainText() override;
	QString selectedText() override;
    int resetFindIndicators(const QString &findText, bool visibility = true) override;
	bool find(const QString &, bool findNext = false, bool findBackwards = false) override;
	void replaceSelectedText(const QString &newText) override;	
	void replaceAll(const QString &findText, const QString &replaceText) override;
	bool findString(const QString & exp, bool findBackwards) const;
	QStringList colorSchemes() override;
    bool canUndo() override;

public slots:
	void zoomIn() override;
	void zoomOut() override;
	void setContentModified(bool) override;
	bool isContentModified() override;
	void indentSelection() override;
	void unindentSelection() override;
	void commentSelection() override;
	void uncommentSelection() override;
	void setPlainText(const QString&) override;
	void highlightError(int) override;
	void unhighlightLastError() override;
	void setHighlightScheme(const QString&) override;
	void insert(const QString&) override;
	void setText(const QString&) override;
	void undo() override;
	void redo() override;
	void cut() override;
	void copy() override;
	void paste() override;
	void initFont(const QString&, uint) override;
private:
	class QTextEdit *textedit;
	class Highlighter *highlighter;
	QSize initialSizeHint;
};

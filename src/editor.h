#pragma once

#include <QObject>
#include <QString>
#include <QWidget>
#include <QWheelEvent>
#include <QScrollBar>
#include <QTextEdit>

class EditorInterface : public QWidget
{
	Q_OBJECT
public:
	EditorInterface(QWidget *parent) : QWidget(parent) {}
	virtual ~EditorInterface() {}
	virtual QSize sizeHint(){ QSize size; return size;}
	virtual void setInitialSizeHint(const QSize&) { }
	virtual void wheelEvent(QWheelEvent*);
	virtual QString toPlainText() = 0;
	virtual QTextDocument *document(){QTextDocument *t = new QTextDocument; return t;}
	virtual QString selectedText() = 0;
	virtual bool find(const QString &, bool findNext = false, bool findBackwards = false) = 0;
	virtual void replaceSelectedText(const QString &newText) = 0;
	virtual void replaceAll(const QString &findText, const QString &replaceText) = 0;
	virtual QStringList colorSchemes() = 0;
    virtual bool canUndo() = 0;

signals:
  void contentsChanged();
  void modificationChanged(bool);												

public slots:
	virtual void zoomIn() = 0;
	virtual void zoomOut() = 0;
	virtual void setContentModified(bool) = 0;
	virtual bool isContentModified() = 0;
	virtual void indentSelection() = 0;
	virtual void unindentSelection() = 0;
	virtual void commentSelection() = 0;
	virtual void uncommentSelection() = 0;
	virtual void setPlainText(const QString &) = 0;
	virtual void highlightError(int) = 0;
	virtual void unhighlightLastError() = 0;
	virtual void setHighlightScheme(const QString&) = 0;
	virtual void insert(const QString&) = 0;
	virtual void setText(const QString&) = 0;
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual void cut() = 0;
	virtual void copy() = 0;
	virtual void paste() = 0;
	virtual void initFont(const QString&, uint) = 0;

private:
	QSize initialSizeHint;
};

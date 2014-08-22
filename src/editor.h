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
	virtual void wheelEvent (QWheelEvent*) { }
	virtual void setTabStopWidth(int) { }
	virtual QString toPlainText() { QString s; return s;}
	virtual QTextDocument *document(){QTextDocument *t = new QTextDocument; return t;}
	virtual bool find(const QString &, bool findNext = false, bool findBackwards = false) = 0;
	virtual void replaceSelectedText(const QString &) = 0;

public slots:
	virtual void zoomIn(){ }
	virtual void zoomOut() { }
	virtual void setLineWrapping(bool) { }
	virtual void setContentModified(bool){ }
	virtual bool isContentModified(){ return 0; } 
	virtual void indentSelection(){ }
	virtual void unindentSelection(){ }
	virtual void commentSelection() {}
	virtual void uncommentSelection(){}
	virtual void setPlainText(const QString &){ }
	virtual void highlightError(int) {}
	virtual void unhighlightLastError() {}
	virtual void setHighlightScheme(const QString&){ }
	virtual void insert(const QString&) = 0;
	virtual void undo(){ }
	virtual void redo(){ }
	virtual void cut(){ }
	virtual void copy(){ }
	virtual void paste(){ }
	virtual void onTextChanged() { }
	virtual void initFont(const QString&, uint){ }

private:
	QSize initialSizeHint;
};

#pragma once

#include <QObject>
#include <QString>
#include <QWidget>
#include <QWheelEvent>
#include <QScrollBar>
#include <QTextEdit>
#include "highlighter.h"

class EditorInterface : public QWidget
{
	Q_OBJECT
public:
	EditorInterface(QWidget *parent);
	~EditorInterface();
        virtual QSize sizeHint(){ QSize size; return size;}
        virtual void setInitialSizeHint(const QSize&) { }
	virtual void wheelEvent (QWheelEvent*) { }
        virtual void setTabStopWidth(int) { }
        virtual QString toPlainText() { QString s; return s;}
        virtual QTextCursor textCursor() { QTextCursor c; return c;}
        virtual void setTextCursor (const QTextCursor &) { }
        virtual QTextDocument *document(){QTextDocument *t = new QTextDocument; return t;}
	virtual bool find(const QString &, bool findNext = false, bool findBackwards = false) = 0;
	virtual void replaceSelectedText(QString&){ }

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
	virtual void insertPlainText(const QString&){ }
	virtual void undo(){ }
        virtual void redo(){ }
        virtual void cut(){ }
        virtual void copy(){ }
        virtual void paste(){ }
	virtual void onTextChanged() { }
	virtual void initFont(const QString&, uint){ }
private:
        Highlighter *highlighter;
	QSize initialSizeHint;

};

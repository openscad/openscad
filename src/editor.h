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
        virtual void setInitialSizeHint(const QSize &size) { }
	virtual void wheelEvent ( QWheelEvent * event ) { }
        virtual void setTabStopWidth(int width) { }
        virtual QString toPlainText() { QString s; return s;}
        virtual QTextCursor textCursor() { QTextCursor c; return c;}
        virtual void setTextCursor (const QTextCursor &cursor) { }
        virtual QTextDocument *document(){QTextDocument *t = new QTextDocument; return t;}
        virtual bool find(const QString & exp, QTextDocument::FindFlags options = 0){ return options;}

public slots:
	virtual void zoomIn(){ }
        virtual void zoomOut() { }
        virtual void setLineWrapping(bool on) { }
        virtual void setContentModified(bool y){ }
        virtual bool isContentModified(){ return true; } 
        virtual void indentSelection(){ }
        virtual void unindentSelection(){ }
        virtual void commentSelection() {}
        virtual void uncommentSelection(){}
        virtual void setPlainText(const QString &text){ }
        virtual void highlightError(int error_pos) {}
        virtual void unhighlightLastError() {}
        virtual void setHighlightScheme(const QString &name){ }
	virtual void insertPlainText(const QString &text){ }
	virtual void undo(){ }
        virtual void redo(){ }
        virtual void cut(){ }
        virtual void copy(){ }
        virtual void paste(){ }
	virtual void onTextChanged() { }
private:
        Highlighter *highlighter;
	QSize initialSizeHint;

};

#ifndef SCINTILLAEDITOR_H
#define SCINTILLAEDITOR_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <Qsci/qsciscintilla.h>
#include <QVBoxLayout>

#include "editor.h"

class ScintillaEditor : public EditorInterface
{
public:
    ScintillaEditor(QWidget *parent);
    QsciScintilla *qsci;
/*	virtual QSize sizeHint(){ QSize size; return size;}
        virtual void setInitialSizeHint(const QSize &size) { }
	virtual void wheelEvent ( QWheelEvent * event ) { }
        virtual void setTabStopWidth(int width) { }
        virtual QString toPlainText() { QString s; return s;}
        virtual QTextCursor textCursor() { QTextCursor c; return c;}
        virtual void setTextCursor (const QTextCursor &cursor) { }
        virtual QTextDocument *document(){QTextDocument *t = new QTextDocument; return t;}
        virtual bool find(const QString & exp, QTextDocument::FindFlags options = 0){ return options;}
*/
public slots:
	 void zoomIn();
         void zoomOut();
  //      virtual void setLineWrapping(bool on) { }
  //      virtual void setContentModified(bool y){ }
  //      virtual bool isContentModified(){ return true; } 
         void indentSelection();
         void unindentSelection();
         void commentSelection();
         void uncommentSelection();
         void setPlainText(const QString &text);
         void highlightError(int error_pos);
         void unhighlightLastError();
         void setHighlightScheme(const QString &name);
	 void insertPlainText(const QString &text);
	 void undo();
         void redo();
         void cut();
         void copy();
         void paste();
private:
    //    Highlighter *highlighter;
 //	QSize initialSizeHint;
   QVBoxLayout *scintillaLayout;
};

#endif // SCINTILLAEDITOR_H

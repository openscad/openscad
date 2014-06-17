#ifndef SCINTILLAEDITOR_H
#define SCINTILLAEDITOR_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <Qsci/qsciscintilla.h>
#include <QVBoxLayout>
#include "scintillahighlighter.h"
#include "editor.h"

class ScintillaEditor : public EditorInterface
{
public:
    ScintillaEditor(QWidget *parent);
    QsciScintilla *qsci;
        QString toPlainText();
	void initFont();
	void initMargin();
	void initLexer();
	//QsciDocument *document() { return qsci->document();}

public slots:
	 void zoomIn();
         void zoomOut(); 
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
	 void onTextChanged();
private:
         QVBoxLayout *scintillaLayout;
	const int indicatorNumber = 1;
	const int markerNumber = 2;
	ScintillaHighlighter *highlighter;
};

#endif // SCINTILLAEDITOR_H

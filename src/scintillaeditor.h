#ifndef SCINTILLAEDITOR_H
#define SCINTILLAEDITOR_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>

#include <Qsci/qsciscintilla.h>

#include "editor.h"
#include "scadapi.h"
#include "scadlexer.h"

class ScintillaEditor : public EditorInterface
{
    Q_OBJECT
    
public:
    ScintillaEditor(QWidget *parent);
    QsciScintilla *qsci;
        QString toPlainText();
	void initFont();
	void initMargin();
	void initLexer();
	void forLightBackground();
	void forDarkBackground();
	void Monokai();
	void Solarized_light();
	void noColor();
	bool findFirst(const QString&, bool, bool, bool, bool, bool, int, int, bool, bool);
	bool findNext();
	void replaceSelectedText(QString&);
	
public slots:
	 void zoomIn();
         void zoomOut(); 
         void indentSelection();
         void unindentSelection();
         void commentSelection();
         void uncommentSelection();
         void setPlainText(const QString&);
	 bool isContentModified();
         void highlightError(int);
         void unhighlightLastError();
         void setHighlightScheme(const QString&);
	 void insertPlainText(const QString&);
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
	ScadLexer *lexer;
        ScadApi *api;
	QString preferenceEditorOption;
};

#endif // SCINTILLAEDITOR_H

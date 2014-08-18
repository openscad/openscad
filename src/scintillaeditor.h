#ifndef SCINTILLAEDITOR_H
#define SCINTILLAEDITOR_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <Qsci/qsciscintilla.h>
#include <QVBoxLayout>
#include "editor.h"
#include "scadlexer.h"

class ScintillaEditor : public EditorInterface
{
public:
    	ScintillaEditor(QWidget *parent);
    	QsciScintilla *qsci;
        QString toPlainText();
	void initMargin();
	void initLexer();
	void forLightBackground();
	void forDarkBackground();
	void Monokai();
	void Solarized_light();
	void noColor();
	bool find(const QString &, bool findNext = false, bool findBackwards = false);
	void replaceSelectedText(QString&);
	
public slots:
	void zoomIn();
        void zoomOut();  
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
	void initFont(const QString&, uint);

private:
        QVBoxLayout *scintillaLayout;
	const int indicatorNumber = 1;
	const int markerNumber = 2;
	ScadLexer *lexer;
	QString preferenceEditorOption;
};

#endif // SCINTILLAEDITOR_H

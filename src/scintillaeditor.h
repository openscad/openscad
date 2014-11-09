#pragma once

#include <QObject>
#include <QWidget>
#include <QMimeData>
#include <QDropEvent>
#include <QVBoxLayout>
#include <Qsci/qsciscintilla.h>
#include <QVBoxLayout>
#include "editor.h"
#include "scadlexer.h"

class QScintilla : public QsciScintilla
{
    Q_OBJECT;
public:
    QScintilla(QWidget *parent);
    virtual void insertAtPos(const QPoint&, const QString&);

public slots:    
    void dropEvent(QDropEvent *event);

signals:
    void fileDropped(const QString &);
};

class ScintillaEditor : public EditorInterface
{
	Q_OBJECT;
public:
	ScintillaEditor(QWidget *parent);
	virtual ~ScintillaEditor() {}
	QScintilla *qsci;
	QString toPlainText();
	void initMargin();
	void initLexer();
	void forLightBackground();
	void forDarkBackground();
	void Monokai();
	void Solarized_light();
	void noColor();
	QString selectedText();
	bool find(const QString &, bool findNext = false, bool findBackwards = false);
	void replaceSelectedText(const QString&);
        
private:
        void get_range(int *lineFrom, int *lineTo);
	
public slots:
	void zoomIn();
	void zoomOut();  
	void setPlainText(const QString&);
	void setContentModified(bool);
	bool isContentModified();
	void highlightError(int);
	void unhighlightLastError();
	void setHighlightScheme(const QString&);
	void indentSelection();
	void unindentSelection();
	void commentSelection();
	void uncommentSelection();
	void insert(const QString&);
        void insertAt(const QPoint&, const QString&);
        void replaceAll(const QString&);
	void undo();
	void redo();
	void cut();
	void copy();
	void paste();
	void initFont(const QString&, uint);
 
private slots:
	void onTextChanged();

private:
	QVBoxLayout *scintillaLayout;
	static const int indicatorNumber = 1;
	static const int markerNumber = 2;
	ScadLexer *lexer;
};

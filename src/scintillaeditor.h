#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <Qsci/qsciscintilla.h>
#include <QVBoxLayout>
#include "editor.h"
#include "scadlexer.h"
#include "parsersettings.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class ScintillaEditor : public EditorInterface
{
	Q_OBJECT;
public:
	ScintillaEditor(QWidget *parent);
	virtual ~ScintillaEditor() {}
	QsciScintilla *qsci;
	QString toPlainText();
	void initMargin();
	void initLexer();
	void noColor();
	QString selectedText();
	bool find(const QString &, bool findNext = false, bool findBackwards = false);
	void replaceSelectedText(const QString&);
        
private:
        void get_range(int *lineFrom, int *lineTo);
        void read_colormap(const boost::filesystem::path path);
        int read_int(boost::property_tree::ptree &pt, const std::string name, const int defaultValue);
        QColor read_color(boost::property_tree::ptree &pt, const std::string name, const QColor defaultColor);
	
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

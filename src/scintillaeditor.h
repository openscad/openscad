#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <Qsci/qsciscintilla.h>
#include <QVBoxLayout>
#include "editor.h"
#include "scadlexer.h"
#include "parsersettings.h"

#include <boost/shared_ptr.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class EditorColorScheme
{
private:
        const fs::path path;
        
        boost::property_tree::ptree pt;
        QString _name;
        int _index;
        
public:
        EditorColorScheme(const fs::path path);
        virtual ~EditorColorScheme();
        
        const QString & name() const;
        int index() const;
        bool valid() const;
        const boost::property_tree::ptree & propertyTree() const;
        
};

class ScintillaEditor : public EditorInterface
{        
	Q_OBJECT;

        typedef std::multimap<int, boost::shared_ptr<EditorColorScheme>, std::less<int> > colorscheme_set_t;
        
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
	QStringList colorSchemes();
        
private:
        void getRange(int *lineFrom, int *lineTo);
        void setColormap(const EditorColorScheme *colorScheme);
        int readInt(const boost::property_tree::ptree &pt, const std::string name, const int defaultValue);
        std::string readString(const boost::property_tree::ptree &pt, const std::string name, const std::string defaultValue);
        QColor readColor(const boost::property_tree::ptree &pt, const std::string name, const QColor defaultColor);
        void enumerateColorSchemesInPath(colorscheme_set_t &result_set, const fs::path path);
        colorscheme_set_t enumerateColorSchemes();
	
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
        void replaceAll(const QString&);
	void undo();
	void redo();
	void cut();
	void copy();
	void paste();
	void initFont(const QString&, uint);

private slots:
	void onTextChanged();
        void applySettings();

private:
	QVBoxLayout *scintillaLayout;
	static const int indicatorNumber = 1;
	static const int markerNumber = 2;
	ScadLexer *lexer;
};

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

#include "memory.h"
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

        typedef std::multimap<int, shared_ptr<EditorColorScheme>, std::less<int>> colorscheme_set_t;
        
public:
	ScintillaEditor(QWidget *parent);
	~ScintillaEditor() {}
	QsciScintilla *qsci;
	QString toPlainText() override;
	void initMargin();
	void initLexer();
	void noColor();
	QString selectedText() override;
	int resetFindIndicators(const QString &findText, bool visibility = true) override;
    bool find(const QString &, bool findNext = false, bool findBackwards = false) override;
	void replaceSelectedText(const QString&) override;
	void replaceAll(const QString &findText, const QString &replaceText) override;
	QStringList colorSchemes() override;
    bool canUndo() override;

private:
        void getRange(int *lineFrom, int *lineTo);
        void setColormap(const EditorColorScheme *colorScheme);
        int readInt(const boost::property_tree::ptree &pt, const std::string name, const int defaultValue);
        std::string readString(const boost::property_tree::ptree &pt, const std::string name, const std::string defaultValue);
        QColor readColor(const boost::property_tree::ptree &pt, const std::string name, const QColor defaultColor);
        void enumerateColorSchemesInPath(colorscheme_set_t &result_set, const fs::path path);
        colorscheme_set_t enumerateColorSchemes();

        bool eventFilter(QObject* obj, QEvent *event) override;
        void navigateOnNumber(int key);
        bool modifyNumber(int key);

signals:
	void previewRequest(void);
	
public slots:
	void zoomIn() override;
	void zoomOut() override;  
	void setPlainText(const QString&) override;
	void setContentModified(bool) override;
	bool isContentModified() override;
	void highlightError(int) override;
	void unhighlightLastError() override;
	void setHighlightScheme(const QString&) override;
	void indentSelection() override;
	void unindentSelection() override;
	void commentSelection() override;
	void uncommentSelection() override;
	void insert(const QString&) override;
	void setText(const QString&) override;
	void undo() override;
	void redo() override;
	void cut() override;
	void copy() override;
	void paste() override;
	void initFont(const QString&, uint) override;

private slots:
	void onTextChanged();
        void applySettings();

private:
	QVBoxLayout *scintillaLayout;
    static const int errorIndicatorNumber = 8; // first 8 are used by lexers 
    static const int findIndicatorNumber = 9; 
	static const int markerNumber = 2;
	ScadLexer *lexer;
	QFont currentFont;
};

#pragma once

#include <functional>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <QMap>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QVBoxLayout>
#include <Qsci/qsciscintilla.h>

#include "editor.h"
#include "memory.h"
#include "scadapi.h"
#include "scadlexer.h"
#include "parsersettings.h"

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

	const QString &name() const;
	int index() const;
	bool valid() const;
	const boost::property_tree::ptree &propertyTree() const;
};

class ScintillaEditor : public EditorInterface
{
	Q_OBJECT;

	using colorscheme_set_t = std::multimap<int, shared_ptr<EditorColorScheme>, std::less<int>>;

public:
	ScintillaEditor(QWidget *parent);
	~ScintillaEditor() {}
	QsciScintilla *qsci;
	QString toPlainText() override;
	void initMargin();
	void initLexer();

	QString selectedText() override;
	int updateFindIndicators(const QString &findText, bool visibility = true) override;
	bool find(const QString &, bool findNext = false, bool findBackwards = false) override;
	void replaceSelectedText(const QString &) override;
	void replaceAll(const QString &findText, const QString &replaceText) override;
	QStringList colorSchemes() override;
	bool canUndo() override;
	void addTemplate() override;
	void setIndicator(const std::vector<IndicatorData> &indicatorData) override;
	QMenu *createStandardContextMenu() override;
	QPoint mapToGlobal(const QPoint &) override;

	void setCursorPosition(int line, int col) override;
	void setFocus() override;
	void setupAutoComplete(const bool forceOff = false);

private:
	void getRange(int *lineFrom, int *lineTo);
	void setColormap(const EditorColorScheme *colorScheme);
	int readInt(const boost::property_tree::ptree &pt, const std::string name,
							const int defaultValue);
	std::string readString(const boost::property_tree::ptree &pt, const std::string name,
												 const std::string defaultValue);
	QColor readColor(const boost::property_tree::ptree &pt, const std::string name,
									 const QColor defaultColor);
	void enumerateColorSchemesInPath(colorscheme_set_t &result_set, const fs::path path);
	colorscheme_set_t enumerateColorSchemes();

	bool eventFilter(QObject *obj, QEvent *event) override;
	bool handleKeyEventNavigateNumber(QKeyEvent *);
	bool handleWheelEventNavigateNumber(QWheelEvent *);
	bool handleKeyEventBlockCopy(QKeyEvent *);
	bool handleKeyEventBlockMove(QKeyEvent *);
	void navigateOnNumber(int key);
	bool modifyNumber(int key);
	void noColor();

	void setLexer(ScadLexer *lexer);
	void replaceSelectedText(QString &);
	void addTemplate(const fs::path path);
	void updateSymbolMarginVisibility();
	void findMarker(int, int, std::function<int(int)>);

signals:
	void previewRequest(void);
	void hyperlinkIndicatorClicked(int val);
	void uriDropped(const QUrl&);

public slots:
	void zoomIn() override;
	void zoomOut() override;
	void setPlainText(const QString &) override;
	void setContentModified(bool) override;
	bool isContentModified() override;
	void highlightError(int) override;
	void unhighlightLastError() override;
	void setHighlightScheme(const QString &) override;
	void indentSelection() override;
	void unindentSelection() override;
	void commentSelection() override;
	void uncommentSelection() override;
	void insert(const QString &) override;
	void setText(const QString &) override;
	void undo() override;
	void redo() override;
	void cut() override;
	void copy() override;
	void paste() override;
	void initFont(const QString &, uint) override;
	void displayTemplates() override;
	void toggleBookmark() override;
	void nextBookmark() override;
	void prevBookmark() override;
	void jumpToNextError() override;

private slots:
	void onTextChanged();
	void onUserListSelected(const int id, const QString &text);
	void applySettings();
	void onAutocompleteChanged(bool state);
	void onCharacterThresholdChanged(int val);
	void fireModificationChanged(bool);
	void onIndicatorClicked(int line, int col, Qt::KeyboardModifiers state);

public:
	void public_applySettings();

private:
	QVBoxLayout *scintillaLayout;
	static const int symbolMargin = 1;
	static const int numberMargin = 0;
	static const int errorIndicatorNumber = 8; // first 8 are used by lexers
	static const int findIndicatorNumber = 9;
	static const int hyperlinkIndicatorNumber = 10;
	static const int hyperlinkIndicatorOffset = 100;
	static const int errMarkerNumber = 2;
	static const int bmMarkerNumber = 3;

	ScadLexer *lexer;
	QFont currentFont;
	ScadApi *api;
	QStringList userList;
	QMap<QString, ScadTemplate> templateMap;
	static const QString cursorPlaceHolder;
};

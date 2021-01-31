#include <ciso646> // C alternative tokens (xor)
#include <stdlib.h>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "boost-utils.h"
#include <QString>
#include <QChar>
#include <QShortcut>
#include <Qsci/qscicommandset.h>

#include "scintillaeditor.h"
#include "Preferences.h"
#include "PlatformUtils.h"
#include "settings.h"
#include "QSettingsCached.h"

#include <QWheelEvent>
#include<QPoint>

namespace fs=boost::filesystem;

const QString ScintillaEditor::cursorPlaceHolder = "^~^";

class SettingsConverter {
public:
	QsciScintilla::WrapMode toWrapMode(const Value &val);
	QsciScintilla::WrapVisualFlag toLineWrapVisualization(const Value &val);
	QsciScintilla::WrapIndentMode toLineWrapIndentationStyle(const Value &val);
	QsciScintilla::WhitespaceVisibility toShowWhitespaces(const Value &val);
};

QsciScintilla::WrapMode SettingsConverter::toWrapMode(const Value &val)
{
	auto strVal = val.toString();
	if (strVal == "Char") {
		return QsciScintilla::WrapCharacter;
	} else if (strVal == "Word") {
		return QsciScintilla::WrapWord;
	} else {
		return QsciScintilla::WrapNone;
	}
}

QsciScintilla::WrapVisualFlag SettingsConverter::toLineWrapVisualization(const Value &val)
{
	auto strVal = val.toString();
	if (strVal == "Text") {
		return QsciScintilla::WrapFlagByText;
	} else if (strVal == "Border") {
		return QsciScintilla::WrapFlagByBorder;
#if QSCINTILLA_VERSION >= 0x020700
	} else if (strVal == "Margin") {
		return QsciScintilla::WrapFlagInMargin;
#endif
	} else {
		return QsciScintilla::WrapFlagNone;
	}
}

QsciScintilla::WrapIndentMode SettingsConverter::toLineWrapIndentationStyle(const Value &val)
{
	auto strVal = val.toString();
	if (strVal == "Same") {
		return QsciScintilla::WrapIndentSame;
	} else if (strVal == "Indented") {
		return QsciScintilla::WrapIndentIndented;
	} else {
		return QsciScintilla::WrapIndentFixed;
	}
}

QsciScintilla::WhitespaceVisibility SettingsConverter::toShowWhitespaces(const Value &val)
{
	auto strVal = val.toString();
	if (strVal == "Always") {
		return QsciScintilla::WsVisible;
	} else if (strVal == "AfterIndentation") {
		return QsciScintilla::WsVisibleAfterIndent;
	} else {
		return QsciScintilla::WsInvisible;
	}
}

EditorColorScheme::EditorColorScheme(fs::path path) : path(path)
{
	try {
		boost::property_tree::read_json(path.generic_string(), pt);
		_name = QString::fromStdString(pt.get<std::string>("name"));
		_index = pt.get<int>("index");
	} catch (const std::exception & e) {
		LOG(message_group::None,Location::NONE,"","Error reading color scheme file '%1$s': %2$s",path.generic_string(),e.what());
		_name = "";
		_index = 0;
	}
}

EditorColorScheme::~EditorColorScheme()
{

}

bool EditorColorScheme::valid() const
{
	return !_name.isEmpty();
}

const QString & EditorColorScheme::name() const
{
	return _name;
}

int EditorColorScheme::index() const
{
	return _index;
}

const boost::property_tree::ptree & EditorColorScheme::propertyTree() const
{
	return pt;
}

ScintillaEditor::ScintillaEditor(QWidget *parent) : EditorInterface(parent)
{
	api = nullptr;
	lexer = nullptr;
	scintillaLayout = new QVBoxLayout(this);
	qsci = new QsciScintilla(this);

	contentsRendered = false;
	findState = 0; //FIND_HIDDEN
	filepath = "";

	// Force EOL mode to Unix, since QTextStream will manage local EOL modes.
	qsci->setEolMode(QsciScintilla::EolUnix);

	//
	// Remapping some scintilla key binding which conflict with OpenSCAD global
	// key bindings, as well as some minor scintilla bugs
	//
	QsciCommand *c;
#ifdef Q_OS_MAC
	// Alt-Backspace should delete left word (Alt-Delete already deletes right word)
	c = qsci->standardCommands()->find(QsciCommand::DeleteWordLeft);
	c->setKey(Qt::Key_Backspace | Qt::ALT);
#endif
	// Cmd/Ctrl-T is handled by the menu
	c = qsci->standardCommands()->boundTo(Qt::Key_T | Qt::CTRL);
	c->setKey(0);
	// Cmd/Ctrl-D is handled by the menu
	c = qsci->standardCommands()->boundTo(Qt::Key_D | Qt::CTRL);
	c->setKey(0);
	// Ctrl-Shift-Z should redo on all platforms
	c = qsci->standardCommands()->find(QsciCommand::Redo);
	c->setKey(Qt::Key_Z | Qt::CTRL | Qt::SHIFT);
	c->setAlternateKey(Qt::Key_Y | Qt::CTRL);

#ifdef Q_OS_MAC
	const unsigned long modifier = Qt::META;
#else
	const unsigned long modifier = Qt::CTRL;
#endif

	QShortcut *shortcutCalltip;
	shortcutCalltip = new QShortcut(modifier | Qt::SHIFT | Qt::Key_Space, this);
	connect(shortcutCalltip, &QShortcut::activated, [=]() { qsci->callTip(); });

	QShortcut *shortcutAutocomplete;
	shortcutAutocomplete = new QShortcut(modifier | Qt::Key_Space, this);
	connect(shortcutAutocomplete, &QShortcut::activated, [=]() { qsci->autoCompleteFromAll(); });

	scintillaLayout->setContentsMargins(0, 0, 0, 0);
	scintillaLayout->addWidget(qsci);

	qsci->setUtf8(true);
	qsci->setFolding(QsciScintilla::BoxedTreeFoldStyle, 4);
	qsci->setCaretLineVisible(true);

	qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, errorIndicatorNumber);
	qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator , findIndicatorNumber);
	qsci->markerDefine(QsciScintilla::Circle, errMarkerNumber);
	qsci->markerDefine(QsciScintilla::Bookmark, bmMarkerNumber);

	qsci->setMarginType(numberMargin, QsciScintilla::NumberMargin);
	qsci->setMarginLineNumbers(numberMargin, true);
	qsci->setMarginMarkerMask(numberMargin, 0);

	qsci->setMarginType(symbolMargin, QsciScintilla::SymbolMargin);
	qsci->setMarginLineNumbers(symbolMargin, false);
	qsci->setMarginWidth(symbolMargin, 0);
	qsci->setMarginMarkerMask(symbolMargin, 1 << errMarkerNumber | 1 << bmMarkerNumber);

	setLexer(new ScadLexer(this));
	initMargin();

	connect(qsci, SIGNAL(textChanged()), this, SIGNAL(contentsChanged()));
	connect(qsci, SIGNAL(modificationChanged(bool)), this, SLOT(fireModificationChanged(bool)));
	connect(qsci, SIGNAL(userListActivated(int, const QString &)), this, SLOT(onUserListSelected(const int, const QString &)));
	qsci->installEventFilter(this);
	qsci->viewport()->installEventFilter(this);

    qsci->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(qsci, SIGNAL(customContextMenuRequested(const QPoint &)), this, SIGNAL(showContextMenuEvent(const QPoint &)));

	qsci->indicatorDefine(QsciScintilla::ThinCompositionIndicator, hyperlinkIndicatorNumber);
	qsci->setIndicatorHoverStyle(QsciScintilla::DotBoxIndicator, hyperlinkIndicatorNumber);
	connect(qsci, SIGNAL(indicatorClicked(int, int, Qt::KeyboardModifiers)), this, SLOT(onIndicatorClicked(int, int, Qt::KeyboardModifiers)));

#if QSCINTILLA_VERSION >= 0x020b00
	connect(qsci, SIGNAL(SCN_URIDROPPED(const QUrl&)), this, SIGNAL(uriDropped(const QUrl&)));
#endif

    // Disabling buffered drawing resolves non-integer HiDPI scaling.
    qsci->SendScintilla(QsciScintillaBase::SCI_SETBUFFEREDDRAW, false);
}

QPoint ScintillaEditor::mapToGlobal(const QPoint &pos)
{
	return qsci->mapToGlobal(pos);
}

QMenu * ScintillaEditor::createStandardContextMenu()
{
	return qsci->createStandardContextMenu();
}

void ScintillaEditor::addTemplate()
{
	addTemplate(PlatformUtils::resourceBasePath());
	addTemplate(PlatformUtils::userConfigPath());
	for(auto key: templateMap.keys())
	{
		userList.append(key);
	}
}

void ScintillaEditor::addTemplate(const fs::path path)
{
	const auto template_path = path / "templates";

	if (fs::exists(template_path) && fs::is_directory(template_path)) {
		for (const auto& dirEntry : boost::make_iterator_range(fs::directory_iterator{template_path}, {})) {
			if (!fs::is_regular_file(dirEntry.status())) continue;

			const auto &path = dirEntry.path();
			if (!(path.extension() == ".json")) continue;

			boost::property_tree::ptree pt;
			try {
				boost::property_tree::read_json(path.generic_string().c_str(), pt);
				const QString key = QString::fromStdString(pt.get<std::string>("key"));
				const QString content = QString::fromStdString(pt.get<std::string>("content"));
				const int cursor_offset = pt.get<int>("offset", -1);

				templateMap.insert(key, ScadTemplate(content, cursor_offset));
			} catch (const std::exception & e) {
				LOG(message_group::None,Location::NONE,"","Error reading template file '%1$s': %2$s",path.generic_string(),e.what());
			}
		}
	}
}

void ScintillaEditor::displayTemplates()
{
	qsci->showUserList(1, userList);
}

/**
 * Apply the settings that are changeable in the preferences. This is also
 * called in the event handler from the preferences.
 */
void ScintillaEditor::applySettings()
{
	SettingsConverter conv;
	auto s = Settings::Settings::inst();

	qsci->setIndentationWidth(s->get(Settings::Settings::indentationWidth).toDouble());
	qsci->setTabWidth(s->get(Settings::Settings::tabWidth).toDouble());
	qsci->setWrapMode(conv.toWrapMode(s->get(Settings::Settings::lineWrap)));
	qsci->setWrapIndentMode(conv.toLineWrapIndentationStyle(s->get(Settings::Settings::lineWrapIndentationStyle)));
	qsci->setWrapVisualFlags(conv.toLineWrapVisualization(s->get(Settings::Settings::lineWrapVisualizationEnd)),
		conv.toLineWrapVisualization(s->get(Settings::Settings::lineWrapVisualizationBegin)),
		s->get(Settings::Settings::lineWrapIndentation).toDouble());
	qsci->setWhitespaceVisibility(conv.toShowWhitespaces(s->get(Settings::Settings::showWhitespace)));
	qsci->setWhitespaceSize(s->get(Settings::Settings::showWhitespaceSize).toDouble());
	qsci->setAutoIndent(s->get(Settings::Settings::autoIndent).toBool());
	qsci->setBackspaceUnindents(s->get(Settings::Settings::backspaceUnindents).toBool());

	auto indentStyle = s->get(Settings::Settings::indentStyle).toString();
	qsci->setIndentationsUseTabs(indentStyle == "Tabs");
	auto tabKeyFunction = s->get(Settings::Settings::tabKeyFunction).toString();
	qsci->setTabIndents(tabKeyFunction == "Indent");

	qsci->setBraceMatching(s->get(Settings::Settings::enableBraceMatching).toBool() ? QsciScintilla::SloppyBraceMatch : QsciScintilla::NoBraceMatch);
	qsci->setCaretLineVisible(s->get(Settings::Settings::highlightCurrentLine).toBool());
	onTextChanged();

	setupAutoComplete(false);
}

void ScintillaEditor::setupAutoComplete(const bool forceOff)
{
	if (qsci->isListActive()) {
		qsci->cancelList();
	}

	if (qsci->isCallTipActive()) {
		qsci->SendScintilla(QsciScintilla::SCI_CALLTIPCANCEL);
	}

	const bool configValue = Preferences::inst()->getValue("editor/enableAutocomplete").toBool();
	const bool enable = configValue && !forceOff;

	if (enable) {
		qsci->setAutoCompletionSource(QsciScintilla::AcsAPIs);
		qsci->setAutoCompletionFillupsEnabled(false);
		qsci->setAutoCompletionFillups("(");
		qsci->setCallTipsVisible(10);
		qsci->setCallTipsStyle(QsciScintilla::CallTipsContext);
	} else {
		qsci->setAutoCompletionSource(QsciScintilla::AcsNone);
		qsci->setAutoCompletionFillupsEnabled(false);
		qsci->setCallTipsStyle(QsciScintilla::CallTipsNone);
	}

	int val = Preferences::inst()->getValue("editor/characterThreshold").toInt();
	qsci->setAutoCompletionThreshold(val <= 0 ? 1 : val);
}

void ScintillaEditor::fireModificationChanged(bool b)
{
	emit modificationChanged(b, this);
}

void ScintillaEditor::public_applySettings()
{
	applySettings();
}

void ScintillaEditor::setPlainText(const QString &text)
{
	qsci->setText(text);
	setContentModified(false);
}

QString ScintillaEditor::toPlainText()
{
	return qsci->text();
}

void ScintillaEditor::setContentModified(bool modified)
{
	// FIXME: Due to an issue with QScintilla, we need to do this on the document itself.
	qsci->SCN_SAVEPOINTLEFT();
	qsci->setModified(modified);
}

bool ScintillaEditor::isContentModified()
{
	return qsci->isModified();
}

void ScintillaEditor::highlightError(int error_pos)
{
	int line, index;
	qsci->lineIndexFromPosition(error_pos, &line, &index);
	qsci->fillIndicatorRange(line, index, line, index + 1, errorIndicatorNumber);
	qsci->markerAdd(line, errMarkerNumber);
	updateSymbolMarginVisibility();
}

void ScintillaEditor::unhighlightLastError()
{
	auto totalLength = qsci->text().length();
	int line, index;
	qsci->lineIndexFromPosition(totalLength, &line, &index);
	qsci->clearIndicatorRange(0, 0, line, index, errorIndicatorNumber);
	qsci->markerDeleteAll(errMarkerNumber);
	updateSymbolMarginVisibility();
}

QColor ScintillaEditor::readColor(const boost::property_tree::ptree &pt, const std::string name, const QColor defaultColor)
{
	try {
		const auto val = pt.get<std::string>(name);
		return QColor(val.c_str());
	} catch (const std::exception &e) {
		return defaultColor;
	}
}

std::string ScintillaEditor::readString(const boost::property_tree::ptree &pt, const std::string name, const std::string defaultValue)
{
	try {
		const auto val = pt.get<std::string>(name);
		return val;
	} catch (const std::exception &e) {
		return defaultValue;
	}
}

int ScintillaEditor::readInt(const boost::property_tree::ptree &pt, const std::string name, const int defaultValue)
{
	try {
		const auto val = pt.get<int>(name);
		return val;
	} catch (const std::exception &e) {
		return defaultValue;
	}
}

void ScintillaEditor::setLexer(ScadLexer *newLexer)
{
	delete this->api;
	this->qsci->setLexer(newLexer);
	this->api = new ScadApi(this->qsci, newLexer);
	delete this->lexer;
	this->lexer = newLexer;
}

void ScintillaEditor::setColormap(const EditorColorScheme *colorScheme)
{
	const auto & pt = colorScheme->propertyTree();

	try {
          auto font = this->lexer->font(this->lexer->defaultStyle());
		const QColor textColor(pt.get<std::string>("text").c_str());
		const QColor paperColor(pt.get<std::string>("paper").c_str());

		auto *newLexer = new ScadLexer(this);

		// Keywords must be set before the lexer is attached to QScintilla
		// as they seem to be read and cached at attach time.
		boost::optional<const boost::property_tree::ptree&> keywords = pt.get_child_optional("keywords");
		if (keywords.is_initialized()) {
			newLexer->setKeywords(1, readString(keywords.get(), "keyword-set1", ""));
			newLexer->setKeywords(2, readString(keywords.get(), "keyword-set2", ""));
			newLexer->setKeywords(3, readString(keywords.get(), "keyword-set-doc", ""));
			newLexer->setKeywords(4, readString(keywords.get(), "keyword-set3", ""));
		}

		setLexer(newLexer);

		// All other properties must be set after attaching to QSCintilla so
		// the editor gets the change events and updates itself to match
		newLexer->setFont(font);
		newLexer->setColor(textColor);
		newLexer->setPaper(paperColor);
                // Somehow, the margin font got lost when we deleted the old lexer
                qsci->setMarginsFont(font);

		const auto& colors = pt.get_child("colors");
		newLexer->setColor(readColor(colors, "keyword1", textColor), QsciLexerCPP::Keyword);
		newLexer->setColor(readColor(colors, "keyword2", textColor), QsciLexerCPP::KeywordSet2);
		newLexer->setColor(readColor(colors, "keyword3", textColor), QsciLexerCPP::GlobalClass);
		newLexer->setColor(readColor(colors, "number", textColor), QsciLexerCPP::Number);
		//newLexer->setColor(readColor(colors, "string", textColor), QsciLexerCPP::SingleQuotedString); //currently, we do not support SingleQuotedStrings
		newLexer->setColor(readColor(colors, "string", textColor), QsciLexerCPP::DoubleQuotedString);
		newLexer->setColor(readColor(colors, "operator", textColor), QsciLexerCPP::Operator);
		newLexer->setColor(readColor(colors, "comment", textColor), QsciLexerCPP::Comment);
		newLexer->setColor(readColor(colors, "commentline", textColor), QsciLexerCPP::CommentLine);
		newLexer->setColor(readColor(colors, "commentdoc", textColor), QsciLexerCPP::CommentDoc);
		newLexer->setColor(readColor(colors, "commentdoc", textColor), QsciLexerCPP::CommentLineDoc);
		newLexer->setColor(readColor(colors, "commentdockeyword", textColor), QsciLexerCPP::CommentDocKeyword);

		const auto& caret = pt.get_child("caret");
		qsci->setCaretWidth(readInt(caret, "width", 1));
		qsci->setCaretForegroundColor(readColor(caret, "foreground", textColor));
		qsci->setCaretLineBackgroundColor(readColor(caret, "line-background", paperColor));

		qsci->setMarkerBackgroundColor(readColor(colors, "error-marker", QColor(255, 0, 0, 100)), errMarkerNumber);
		qsci->setMarkerBackgroundColor(readColor(colors, "bookmark-marker", QColor(150, 200, 255, 100)), bmMarkerNumber); // light blue
        qsci->setIndicatorForegroundColor(readColor(colors, "error-indicator", QColor(255, 0, 0, 100)), errorIndicatorNumber);//red
        qsci->setIndicatorOutlineColor(readColor(colors, "error-indicator-outline", QColor(255, 0, 0, 100)), errorIndicatorNumber);//red
        qsci->setIndicatorForegroundColor(readColor(colors, "find-indicator", QColor(255, 255, 0, 100)), findIndicatorNumber);//yellow
        qsci->setIndicatorOutlineColor(readColor(colors, "find-indicator-outline", QColor(255, 255, 0, 100)), findIndicatorNumber);//yellow
        qsci->setIndicatorForegroundColor(readColor(colors, "hyperlink-indicator", QColor(139, 24, 168, 100)), hyperlinkIndicatorNumber);//violet
        qsci->setIndicatorOutlineColor(readColor(colors, "hyperlink-indicator-outline", QColor(139, 24, 168, 100)), hyperlinkIndicatorNumber);//violet
        qsci->setIndicatorHoverForegroundColor(readColor(colors, "hyperlink-indicator-hover", QColor(139, 24, 168, 100)), hyperlinkIndicatorNumber);//violet
		qsci->setWhitespaceForegroundColor(readColor(colors, "whitespace-foreground", textColor));
		qsci->setMarginsBackgroundColor(readColor(colors, "margin-background", paperColor));
		qsci->setMarginsForegroundColor(readColor(colors, "margin-foreground", textColor));
		qsci->setFoldMarginColors(readColor(colors, "margin-background", paperColor),
                                          readColor(colors, "margin-background", paperColor));
		qsci->setMatchedBraceBackgroundColor(readColor(colors, "matched-brace-background", paperColor));
		qsci->setMatchedBraceForegroundColor(readColor(colors, "matched-brace-foreground", textColor));
		qsci->setUnmatchedBraceBackgroundColor(readColor(colors, "unmatched-brace-background", paperColor));
		qsci->setUnmatchedBraceForegroundColor(readColor(colors, "unmatched-brace-foreground", textColor));
		qsci->setSelectionForegroundColor(readColor(colors, "selection-foreground", paperColor));
		qsci->setSelectionBackgroundColor(readColor(colors, "selection-background", textColor));
		qsci->setEdgeColor(readColor(colors, "edge", textColor));
	} catch (const std::exception &e) {
		noColor();
	}
}

void ScintillaEditor::noColor()
{
	this->lexer->setPaper(Qt::white);
	this->lexer->setColor(Qt::black);
	qsci->setCaretWidth(2);
	qsci->setCaretForegroundColor(Qt::black);
	qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), errMarkerNumber);
	qsci->setMarkerBackgroundColor(QColor(150, 200, 255, 100), bmMarkerNumber); // light blue
    qsci->setIndicatorForegroundColor(QColor(255, 0, 0, 128), errorIndicatorNumber);//red
    qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), errorIndicatorNumber); // only alpha part is used
    qsci->setIndicatorForegroundColor(QColor(255, 255, 0, 128), findIndicatorNumber);//yellow
    qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), findIndicatorNumber); // only alpha part is used
    qsci->setIndicatorForegroundColor(QColor(139, 24, 168, 128), hyperlinkIndicatorNumber);//violet
    qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), hyperlinkIndicatorNumber); // only alpha part is used
    qsci->setIndicatorHoverForegroundColor(QColor(139, 24, 168, 128), hyperlinkIndicatorNumber);//violet
	qsci->setCaretLineBackgroundColor(Qt::white);
	qsci->setWhitespaceForegroundColor(Qt::black);
	qsci->setSelectionForegroundColor(Qt::black);
	qsci->setSelectionBackgroundColor(QColor("LightSkyBlue"));
	qsci->setMatchedBraceBackgroundColor(QColor("LightBlue"));
	qsci->setMatchedBraceForegroundColor(Qt::black);
	qsci->setUnmatchedBraceBackgroundColor(QColor("pink"));
	qsci->setUnmatchedBraceForegroundColor(Qt::black);
	qsci->setMarginsBackgroundColor(QColor("whiteSmoke"));
	qsci->setMarginsForegroundColor(QColor("gray"));
	qsci->setFoldMarginColors(QColor("whiteSmoke"), QColor("whiteSmoke"));
	qsci->setEdgeColor(Qt::black);
}

void ScintillaEditor::enumerateColorSchemesInPath(ScintillaEditor::colorscheme_set_t &result_set, const fs::path path)
{
	const auto color_schemes = path / "color-schemes" / "editor";

	if (fs::exists(color_schemes) && fs::is_directory(color_schemes)) {
		for (const auto& dirEntry : boost::make_iterator_range(fs::directory_iterator{color_schemes}, {})) {
			if (!fs::is_regular_file(dirEntry.status())) continue;

			const auto &path = dirEntry.path();
			if (!(path.extension() == ".json")) continue;

			auto colorScheme = std::make_shared<EditorColorScheme>(path);
			if (colorScheme->valid()) {
				result_set.emplace(colorScheme->index(), colorScheme);
			}
		}
	}
}

ScintillaEditor::colorscheme_set_t ScintillaEditor::enumerateColorSchemes()
{
	colorscheme_set_t result_set;

	enumerateColorSchemesInPath(result_set, PlatformUtils::resourceBasePath());
	enumerateColorSchemesInPath(result_set, PlatformUtils::userConfigPath());

	return result_set;
}

QStringList ScintillaEditor::colorSchemes()
{
	QStringList colorSchemes;
	for (const auto &colorSchemeEntry : enumerateColorSchemes()) {
		colorSchemes << colorSchemeEntry.second.get()->name();
	}
	colorSchemes << "Off";

	return colorSchemes;
}

bool ScintillaEditor::canUndo()
{
    return qsci->isUndoAvailable();
}

void ScintillaEditor::setHighlightScheme(const QString &name)
{
	for (const auto &colorSchemeEntry : enumerateColorSchemes()) {
		const auto colorScheme = colorSchemeEntry.second.get();
		if (colorScheme->name() == name) {
			setColormap(colorScheme);
			return;
		}
	}

	noColor();
}

void ScintillaEditor::insert(const QString &text)
{
	qsci->insert(text);
}

void ScintillaEditor::setText(const QString &text)
{
	qsci->selectAll(true);
	qsci->replaceSelectedText(text);
}

void ScintillaEditor::undo()
{
	qsci->undo();
}

void ScintillaEditor::redo()
{
	qsci->redo();
}

void ScintillaEditor::cut()
{
	qsci->cut();
}

void ScintillaEditor::copy()
{
	qsci->copy();
}

void ScintillaEditor::paste()
{
	qsci->paste();
}

void ScintillaEditor::zoomIn()
{
	qsci->zoomIn();
}

void ScintillaEditor::zoomOut()
{
	qsci->zoomOut();
}

void ScintillaEditor::initFont(const QString& fontName, uint size)
{
  this->currentFont = QFont(fontName, size);
  this->currentFont.setFixedPitch(true);
  this->lexer->setFont(this->currentFont);
  qsci->setMarginsFont(this->currentFont);
  onTextChanged(); // Update margin width
}

void ScintillaEditor::initMargin()
{
  connect(qsci, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

void ScintillaEditor::onTextChanged()
{
	auto s = Settings::Settings::inst();
	auto enableLineNumbers = s->get(Settings::Settings::enableLineNumbers).toBool();

	if (enableLineNumbers) {
		qsci->setMarginWidth(numberMargin, QString(trunc(log10(qsci->lines()) + 2), '0'));
	} else {
		qsci->setMarginWidth(numberMargin, 6);
	}
	qsci->setMarginLineNumbers(numberMargin, enableLineNumbers);
}

int ScintillaEditor::updateFindIndicators(const QString &findText, bool visibility)
{
	int findwordcount{0};

	qsci->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, findIndicatorNumber);
	qsci->SendScintilla(qsci->SCI_INDICATORCLEARRANGE, 0, qsci->length());

	const auto txt = qsci->text().toUtf8();
	const auto findTextUtf8 = findText.toUtf8();
	auto pos = txt.indexOf(findTextUtf8);
	auto len = findTextUtf8.length();
	if (visibility && len > 0) {
		while (pos != -1) {
			findwordcount++;
			qsci->SendScintilla(qsci->SCI_SETINDICATORCURRENT, findIndicatorNumber);
			qsci->SendScintilla(qsci->SCI_INDICATORFILLRANGE, pos, len);
			pos = txt.indexOf(findTextUtf8, pos + len);
		};
	}
	return findwordcount;
}

bool ScintillaEditor::find(const QString &expr, bool findNext, bool findBackwards)
{
	int startline = -1, startindex = -1;

	// If findNext, start from the end of the current selection
	if (qsci->hasSelectedText()) {
		int lineFrom, indexFrom, lineTo, indexTo;
		qsci->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

		startline = !(findBackwards xor findNext) ? std::min(lineFrom, lineTo) : std::max(lineFrom, lineTo);
		startindex = !(findBackwards xor findNext) ? std::min(indexFrom, indexTo) : std::max(indexFrom, indexTo);
	}

	return qsci->findFirst(expr, false, false, false, true,
		!findBackwards, startline, startindex);
}

void ScintillaEditor::replaceSelectedText(const QString &newText)
{
    if ((qsci->selectedText() != newText)&&(qsci->hasSelectedText())) qsci->replaceSelectedText(newText);
}

void ScintillaEditor::replaceAll(const QString &findText, const QString &replaceText)
{
  // We need to issue a Select All first due to a bug in QScintilla:
  // It doesn't update the find range when just doing findFirst() + findNext() causing the search
  // to end prematurely if the replaced string is larger than the selected string.
#if QSCINTILLA_VERSION >= 0x020903
  // QScintilla bug seams to be fixed in 2.9.3
  if (qsci->findFirst(findText,
                      false /*re*/, false /*cs*/, false /*wo*/,
                      false /*wrap*/, true /*forward*/, 0, 0)) {
#elif QSCINTILLA_VERSION >= 0x020700
  qsci->selectAll();
  if (qsci->findFirstInSelection(findText, 
                      false /*re*/, false /*cs*/, false /*wo*/, 
                      false /*wrap*/, true /*forward*/)) {
#else
    // findFirstInSelection() was introduced in QScintilla 2.7
  if (qsci->findFirst(findText, 
                      false /*re*/, false /*cs*/, false /*wo*/, 
                      false /*wrap*/, true /*forward*/, 0, 0)) {
#endif
    qsci->replace(replaceText);
    while (qsci->findNext()) {
      qsci->replace(replaceText);
    }
  }
}


void ScintillaEditor::getRange(int *lineFrom, int *lineTo)
{
	int indexFrom, indexTo;
	if (qsci->hasSelectedText()) {
		qsci->getSelection(lineFrom, &indexFrom, lineTo, &indexTo);
		if (indexTo == 0) {
			*lineTo = *lineTo - 1;
		}
	} else {
		qsci->getCursorPosition(lineFrom, &indexFrom);
		*lineTo = *lineFrom;
	}
}

void ScintillaEditor::indentSelection()
{
	int lineFrom, lineTo;
	getRange(&lineFrom, &lineTo);
	for (int line = lineFrom; line <= lineTo; ++line) {
		qsci->indent(line);
	}
}

void ScintillaEditor::unindentSelection()
{
	int lineFrom, lineTo;
	getRange(&lineFrom, &lineTo);
	for (int line = lineFrom; line <= lineTo; ++line) {
		qsci->unindent(line);
	}
}

void ScintillaEditor::commentSelection()
{
	auto hasSelection = qsci->hasSelectedText();

	int lineFrom, lineTo;
	getRange(&lineFrom, &lineTo);
	for (int line = lineFrom; line <= lineTo; ++line) {
		qsci->insertAt("//", line, 0);
	}

	if (hasSelection) {
		qsci->setSelection(lineFrom, 0, lineTo, std::max(0, qsci->lineLength(lineTo) - 1));
	}
}

void ScintillaEditor::uncommentSelection()
{
	auto hasSelection = qsci->hasSelectedText();

	int lineFrom, lineTo;
	getRange(&lineFrom, &lineTo);
	for (int line = lineFrom; line <= lineTo; ++line) {
		QString lineText = qsci->text(line);
		if (lineText.startsWith("//")) {
			qsci->setSelection(line, 0, line, 2);
			qsci->removeSelectedText();
		}
	}
	if (hasSelection) {
		qsci->setSelection(lineFrom, 0, lineTo, std::max(0, qsci->lineLength(lineTo) - 1));
	}
}

QString ScintillaEditor::selectedText()
{
	return qsci->selectedText();
}

bool ScintillaEditor::eventFilter(QObject *obj, QEvent *e)
{
	bool enableNumberScrollWheel = Settings::Settings::inst()->get(Settings::Settings::enableNumberScrollWheel).toBool();

	if(obj == qsci->viewport() && enableNumberScrollWheel)
	{
		if(e->type() == QEvent::Wheel)
		{
			auto *wheelEvent = static_cast <QWheelEvent*> (e);
			PRINTDB("%s - modifier: %s",(e->type()==QEvent::Wheel?"Wheel Event":"")%(wheelEvent->modifiers() & Qt::AltModifier?"Alt":"Other Button"));
			if(handleWheelEventNavigateNumber(wheelEvent))
			{
				qsci->SendScintilla(QsciScintilla::SCI_SETCARETWIDTH, 1);
				return true;
			}
		}
		return false;
	}
	else if(obj == qsci)
	{
		if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease)
		{
		auto *keyEvent = static_cast<QKeyEvent*> (e);

		PRINTDB("%10s - modifiers: %s %s %s %s %s %s",
				(e->type() == QEvent::KeyPress ? "KeyPress" : "KeyRelease") %
				(keyEvent->modifiers() & Qt::ShiftModifier ? "SHIFT" : "shift") %
				(keyEvent->modifiers() & Qt::ControlModifier ? "CTRL" : "ctrl") %
				(keyEvent->modifiers() & Qt::AltModifier ? "ALT" : "alt") %
				(keyEvent->modifiers() & Qt::MetaModifier ? "META" : "meta") %
				(keyEvent->modifiers() & Qt::KeypadModifier ? "KEYPAD" : "keypad") %
				(keyEvent->modifiers() & Qt::GroupSwitchModifier ? "GROUP" : "group"));

			if (handleKeyEventNavigateNumber(keyEvent))
			{
			return true;
		}
			if (handleKeyEventBlockCopy(keyEvent))
			{
			return true;
		}
			if (handleKeyEventBlockMove(keyEvent))
			{
			return true;
		}
	}
	return false;
}
	else return EditorInterface::eventFilter(obj, e);

	return false;
}

bool ScintillaEditor::handleKeyEventBlockMove(QKeyEvent *keyEvent)
{
	unsigned int modifiers = Qt::ControlModifier | Qt::GroupSwitchModifier;

	if (keyEvent->type() != QEvent::KeyRelease) {
		return false;
	}

	if (keyEvent->modifiers() != modifiers) {
		return false;
	}

	if (keyEvent->key() != Qt::Key_Up && keyEvent->key() != Qt::Key_Down) {
		return false;
	}

	int line, index;
	qsci->getCursorPosition(&line, &index);
	int lineFrom, indexFrom, lineTo, indexTo;
	qsci->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
	if (lineFrom < 0) {
		lineTo = lineFrom = line;
		indexFrom = indexTo = 0;
	}
	int selectionLineTo = lineTo;
	if (lineTo > lineFrom && indexTo == 0) {
		lineTo--;
	}

	bool up = keyEvent->key() == Qt::Key_Up;
	int directionOffset = up ? -1 : 1;
	int lineToMove = up ? lineFrom - 1 : lineTo + 1;
	if (lineToMove < 0) {
		return false;
	}

	qsci->beginUndoAction();
	QString textToMove = qsci->text(lineToMove);
	QString text;
	for (int idx = lineFrom; idx <= lineTo; ++idx) {
		text.append(qsci->text(idx));
	}
	if (lineToMove >= qsci->lines() - 1) {
		textToMove.append('\n');
	}
	text.insert(up ? text.length() : 0, textToMove);
	qsci->setSelection(std::min(lineToMove, lineFrom), 0, std::max(lineToMove, lineTo) + 1, 0);
	qsci->replaceSelectedText(text);
	qsci->setCursorPosition(line + directionOffset, index);
	qsci->setSelection(lineFrom + directionOffset, indexFrom, selectionLineTo + directionOffset, indexTo);
	qsci->endUndoAction();
	return true;
}

bool ScintillaEditor::handleKeyEventBlockCopy(QKeyEvent *keyEvent)
{
	unsigned int modifiers = Qt::ControlModifier | Qt::ShiftModifier;

	if (keyEvent->type() != QEvent::KeyRelease) {
		return false;
	}

	if (keyEvent->modifiers() != modifiers) {
		return false;
	}

	if (keyEvent->key() != Qt::Key_Up && keyEvent->key() != Qt::Key_Down) {
		return false;
	}

	int line, index;
	qsci->getCursorPosition(&line, &index);
	int lineFrom, indexFrom, lineTo, indexTo;
	qsci->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
	if (lineFrom < 0) {
		lineTo = lineFrom = line;
		indexFrom = indexTo = 0;
	}

	bool up = keyEvent->key() == Qt::Key_Up;
	int selectionLineTo = 0;
	if (lineTo > lineFrom && indexTo == 0) {
		lineTo--;
		selectionLineTo++;
	}
	int cursorLine = up ? line : lineTo + 1;
	int selectionLineFrom = up ? lineFrom : lineTo + 1;
	selectionLineTo += up ? lineTo : 2 * lineTo - lineFrom + 1;

	qsci->beginUndoAction();
	QString text;
	for (int line = lineFrom; line <= lineTo; ++line) {
		text += qsci->text(line);
	}
	if (lineTo + 1 >= qsci->lines()) {
		text.insert(0, '\n');
	}
	qsci->insertAt(text, lineTo + 1, 0);
	qsci->setCursorPosition(cursorLine, index);
	qsci->setSelection(selectionLineFrom, indexFrom, selectionLineTo, indexTo);
	qsci->endUndoAction();
	return true;
}

bool ScintillaEditor::handleKeyEventNavigateNumber(QKeyEvent *keyEvent)
{
	static bool wasChanged = false;
	static bool previewAfterUndo = false;

#ifdef Q_OS_MAC
	unsigned int navigateOnNumberModifiers = Qt::AltModifier | Qt::ShiftModifier | Qt::KeypadModifier;
#else
	unsigned int navigateOnNumberModifiers = Qt::AltModifier;
#endif
	if (keyEvent->modifiers() == navigateOnNumberModifiers) {
		switch (keyEvent->key()) {
		case Qt::Key_Left:
		case Qt::Key_Right:
			if (keyEvent->type() == QEvent::KeyPress) {
				navigateOnNumber(keyEvent->key());
			}
			return true;

		case Qt::Key_Up:
		case Qt::Key_Down:
			if (keyEvent->type() == QEvent::KeyPress) {
				if (!wasChanged) qsci->beginUndoAction();
				if (modifyNumber(keyEvent->key())) {
					wasChanged = true;
					previewAfterUndo = true;
				}
				if (!wasChanged) qsci->endUndoAction();
			}
			return true;
		}
	}
	if (previewAfterUndo && keyEvent->type() == QEvent::KeyPress) {
		int k = keyEvent->key() | keyEvent->modifiers();
		if (wasChanged) qsci->endUndoAction();
		wasChanged = false;
		auto *cmd = qsci->standardCommands()->boundTo(k);
		if (cmd && (cmd->command() == QsciCommand::Undo || cmd->command() == QsciCommand::Redo))
			QTimer::singleShot(0, this, SIGNAL(previewRequest()));
		else if (cmd || !keyEvent->text().isEmpty()) {
			// any insert or command (but not undo/redo) cancels the preview after undo
			previewAfterUndo = false;
		}
	}
	return false;
}

bool ScintillaEditor::handleWheelEventNavigateNumber (QWheelEvent *wheelEvent)
{
	auto modifierNumberScrollWheel = Settings::Settings::inst()->get(Settings::Settings::modifierNumberScrollWheel).toString();
	bool modifier;	
	static bool wasChanged = false;
	static bool previewAfterUndo = false;

	if(modifierNumberScrollWheel=="Alt")
	{
		modifier = wheelEvent->modifiers() & Qt::AltModifier;
	}
	else if(modifierNumberScrollWheel=="Left Mouse Button")
	{
		modifier = wheelEvent->buttons() & Qt::LeftButton;
	}
	else
	{
		modifier = (wheelEvent->buttons() & Qt::LeftButton) | (wheelEvent->modifiers() & Qt::AltModifier);	
	}

	if (modifier)
	 {
		if (!wasChanged) qsci->beginUndoAction();

		if (wheelEvent->delta() < 0)
		{
			if (modifyNumber(Qt::Key_Down)) 
			{
				wasChanged = true;
				previewAfterUndo = true;
			}
		}
		else
		{
			// wheelEvent->delta() > 0
			if (modifyNumber(Qt::Key_Up))
			{
				wasChanged = true;
				previewAfterUndo = true;
			}
		}
		if (!wasChanged) qsci->endUndoAction();

		return true;
	}

	if (previewAfterUndo)
	{
        int k = wheelEvent->buttons() & Qt::LeftButton;
		if (wasChanged) qsci->endUndoAction();
		wasChanged = false;
		auto *cmd = qsci->standardCommands()->boundTo(k);
		if (cmd && (cmd->command() == QsciCommand::Undo || cmd->command() == QsciCommand::Redo))
			QTimer::singleShot(0, this, SIGNAL(previewRequest()));
		else if (cmd || wheelEvent->delta())
		{
			// any insert or command (but not undo/redo) cancels the preview after undo
			previewAfterUndo = false;
		}
	}
	return false;
}

void ScintillaEditor::navigateOnNumber(int key)
{
	int line, index;
	qsci->getCursorPosition(&line, &index);
	auto text=qsci->text(line);
	auto left=text.left(index);
	auto dotOnLeft=left.contains(QRegExp("\\.\\d*$"));
	auto dotJustLeft=index>1 && text[index-2]=='.';
	auto dotJustRight=text[index]=='.';
	auto numOnLeft=left.contains(QRegExp("\\d\\.?$")) || left.endsWith("-.");
	auto numOnRight=text.indexOf(QRegExp("\\.?\\d"),index)==index;

	switch (key)
	{
		case Qt::Key_Left:
			if (numOnLeft)
				qsci->setCursorPosition(line, index-(dotJustLeft?2:1));
			break;

		case Qt::Key_Right:
			if (numOnRight)
				qsci->setCursorPosition(line, index+(dotJustRight?2:1));
			else if (numOnLeft) {
				// add trailing zero
				if (!dotOnLeft) {
					qsci->insert(".0");
					index++;
				} else {
					qsci->insert("0");
				}
				qsci->setCursorPosition(line, index+1);
			}
			break;
	}
}

bool ScintillaEditor::modifyNumber(int key)
{
	int line, index;
	qsci->getCursorPosition(&line, &index);
	auto text=qsci->text(line);
	int lineFrom, indexFrom, lineTo, indexTo;
	qsci->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
	auto hadSelection=qsci->hasSelectedText();
	qsci->SendScintilla(QsciScintilla::SCI_SETEMPTYSELECTION);
	qsci->setCursorPosition(line, index);

	auto begin=QRegExp("[-+]?\\d*\\.?\\d*$").indexIn(text.left(index));

	QRegExp rx("[_a-zA-Z]");
	auto check = text.mid(begin-1,1);
	if(rx.exactMatch(check)) return false;

	auto end=text.indexOf(QRegExp("[^0-9.]"),index);
	if (end<0) end=text.length();
	auto nr=text.mid(begin,end-begin);
	if ( !(nr.contains(QRegExp("^[-+]?\\d*\\.?\\d+$")) && nr.contains(QRegExp("\\d"))) ) return false;
	auto sign=nr[0]=='+'||nr[0]=='-';
	if (nr.endsWith('.')) nr=nr.left(nr.length()-1);
	auto curpos=index-begin;
	if(curpos==0 || (curpos==1 && (nr[0]=='+' || nr[0]=='-'))) return false;
	auto dotpos=nr.indexOf('.');
	auto decimals=dotpos<0?0:nr.length()-dotpos-1;
	auto number=(dotpos<0)?nr.toLongLong():(nr.left(dotpos)+nr.mid(dotpos+1)).toLongLong();
	auto tail=nr.length()-curpos;
	auto exponent=tail-((dotpos>=curpos)?1:0);
	long long int step=Preferences::inst()->getValue("editor/stepSize").toInt();
	for (int i=exponent; i>0; i--) step*=10;

	switch (key) {
		case Qt::Key_Up:   number+=step; break;
		case Qt::Key_Down: number-=step; break;
	}
	auto negative=number<0;
	if (negative) number=-number;
	auto newnr=QString::number(number);
	if (decimals) {
		if (newnr.length()<=decimals) newnr.prepend(QString(decimals-newnr.length()+1,'0'));
		newnr=newnr.left(newnr.length()-decimals)+"."+newnr.right(decimals);
	}
	if (tail>newnr.length()) {
		newnr.prepend(QString(tail-newnr.length(),'0'));
	}
	if (negative) newnr.prepend('-');
	else if (sign) newnr.prepend('+');
	qsci->setSelection(line, begin, line, end);
	qsci->replaceSelectedText(newnr);

	qsci->selectAll(false);
	if (hadSelection)
	{
		qsci->setSelection(lineFrom, indexFrom, lineTo, indexTo);
	}
	qsci->setCursorPosition(line, begin+newnr.length()-tail);
	emit previewRequest();
	return true;
}

void ScintillaEditor::onUserListSelected(const int, const QString &text)
{
	if (!templateMap.contains(text)) {
		return;
	}

	QString tabReplace = "";
	if(Settings::Settings::inst()->get(Settings::Settings::indentStyle).toString() == "Spaces")
	{
		auto spCount = Settings::Settings::inst()->get(Settings::Settings::indentationWidth).toDouble();
		tabReplace = QString(spCount, ' ');
	}

	ScadTemplate &t = templateMap[text];
	QString content = t.get_text();
	int cursor_offset = t.get_cursor_offset();

	if(cursor_offset < 0)
	{
		if(tabReplace.size() != 0)
			content.replace("\t", tabReplace);

		cursor_offset = content.indexOf(ScintillaEditor::cursorPlaceHolder);
		content.remove(cursorPlaceHolder);

		if(cursor_offset == -1)
			cursor_offset = content.size();
	}
	else
	{
		if(tabReplace.size() != 0)
		{
			int tbCount = content.left(cursor_offset).count("\t");
			cursor_offset += tbCount * (tabReplace.size() - 1);
			content.replace("\t", tabReplace);
		}
	}

	qsci->insert(content);

	int line, index;
	qsci->getCursorPosition(&line, &index);
	int pos = qsci->positionFromLineIndex(line, index);

	pos += cursor_offset;
	int indent_line = line;
	int indent_width = index;
	qsci->lineIndexFromPosition(pos, &line, &index);
	qsci->setCursorPosition(line, index);

	int lines = t.get_text().count("\n");
	QString indent_char = " ";
	if(Settings::Settings::inst()->get(Settings::Settings::indentStyle).toString() == "Tabs")
		indent_char = "\t";

	for (int a = 0; a < lines; ++a) {
		qsci->insertAt(indent_char.repeated(indent_width), indent_line + a + 1, 0);
	}
}

void ScintillaEditor::onAutocompleteChanged(bool state)
{
	if(state)
	{
		qsci->setAutoCompletionSource(QsciScintilla::AcsAPIs);
		qsci->setAutoCompletionFillupsEnabled(true);
		qsci->setCallTipsVisible(10);
		qsci->setCallTipsStyle(QsciScintilla::CallTipsContext);
	}
	else
	{
		qsci->setAutoCompletionSource(QsciScintilla::AcsNone);
		qsci->setAutoCompletionFillupsEnabled(false);
		qsci->setCallTipsStyle(QsciScintilla::CallTipsNone);
	}
}

void ScintillaEditor::onCharacterThresholdChanged(int val)
{
	qsci->setAutoCompletionThreshold(val <= 0 ? 1 : val);
}

void ScintillaEditor::setIndicator(const std::vector<IndicatorData>& indicatorData)
{
	qsci->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, hyperlinkIndicatorNumber);
	qsci->SendScintilla(QsciScintilla::SCI_INDICATORCLEARRANGE, 0, qsci->text().length());
	this->indicatorData = indicatorData;

	int idx = 0;
	for (const auto& data : indicatorData) {
		int pos = qsci->positionFromLineIndex(data.linenr - 1, data.colnr - 1);
		qsci->SendScintilla(QsciScintilla::SCI_SETINDICATORVALUE, idx + hyperlinkIndicatorOffset);
		qsci->SendScintilla(QsciScintilla::SCI_INDICATORFILLRANGE, pos, data.nrofchar);
		idx++;
	}
}

void ScintillaEditor::onIndicatorClicked(int line, int col, Qt::KeyboardModifiers state)
{
	if (!(state == Qt::ControlModifier || state == (Qt::ControlModifier|Qt::AltModifier)))
		return;

	qsci->SendScintilla(QsciScintilla::SCI_SETINDICATORCURRENT, hyperlinkIndicatorNumber);

	int pos = qsci->positionFromLineIndex(line, col);
	int val = qsci->SendScintilla(QsciScintilla::SCI_INDICATORVALUEAT, ScintillaEditor::hyperlinkIndicatorNumber, pos);

	// checking if indicator clicked is hyperlinkIndicator
	if(val >= hyperlinkIndicatorOffset && val <= hyperlinkIndicatorOffset+indicatorData.size())	{
		emit hyperlinkIndicatorClicked(val - hyperlinkIndicatorOffset);
	}
}

void ScintillaEditor::setCursorPosition(int line, int col)
{
	qsci->setCursorPosition(line, col);
}

void ScintillaEditor::updateSymbolMarginVisibility()
{
	if (qsci->markerFindNext(0, 1 << bmMarkerNumber | 1 << errMarkerNumber) < 0) {
		qsci->setMarginWidth(symbolMargin, 0);
	} else {
		qsci->setMarginWidth(symbolMargin, "00");
	}
}

void ScintillaEditor::toggleBookmark()
{
	int line, index;
	qsci->getCursorPosition(&line, &index);

	unsigned int state = qsci->markersAtLine(line);

	if ((state & (1<<bmMarkerNumber))==0)
		qsci->markerAdd(line, bmMarkerNumber);
	else
		qsci->markerDelete(line, bmMarkerNumber);

	updateSymbolMarginVisibility();
}

void ScintillaEditor::findMarker(int findStartOffset, int wrapStart, std::function<int(int)> findMarkerFunc)
{
	int line, index;
	qsci->getCursorPosition(&line, &index);
	line = findMarkerFunc(line + findStartOffset);
	if (line == -1) {
		line = findMarkerFunc(wrapStart); // wrap around
	}
	if (line != -1) {
		// make sure we don't wrap into new line
		int len = qsci->text(line).remove(QRegExp("[\n\r]$")).length();
		int col = std::min(index, len);
		qsci->setCursorPosition(line, col);
	}
}

void ScintillaEditor::nextBookmark()
{
	findMarker(1, 0, [this](int line){ return qsci->markerFindNext(line, 1 << bmMarkerNumber); });
}

void ScintillaEditor::prevBookmark()
{
	findMarker(-1, qsci->lines() - 1, [this](int line){ return qsci->markerFindPrevious(line, 1 << bmMarkerNumber); });
}

void ScintillaEditor::jumpToNextError()
{
	findMarker(1, 0, [this](int line){ return qsci->markerFindNext(line, 1 << errMarkerNumber); });
}

void ScintillaEditor::setFocus()
{
	qsci->setFocus();
	qsci->SendScintilla(QsciScintilla::SCI_SETFOCUS, true);
}

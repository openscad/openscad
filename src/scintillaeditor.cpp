#include <algorithm>
#include <QString>
#include <QChar>
#include "boosty.h"
#include "scintillaeditor.h"
#include <Qsci/qscicommandset.h>
#include "Preferences.h"
#include "PlatformUtils.h"
#include "settings.h"

class SettingsConverter {
public:
	QsciScintilla::WrapMode fromWrapMode(Settings::LineWrap val);
	QsciScintilla::WrapVisualFlag fromLineWrapVisualization(Settings::LineWrapVisualization val);
	QsciScintilla::WrapIndentMode fromLineWrapIndentationStyle(Settings::LineWrapIndentationStyle val);
	QsciScintilla::WhitespaceVisibility fromShowWhitespaces(Settings::ShowWhitespaces val);
};

QsciScintilla::WrapMode SettingsConverter::fromWrapMode(Settings::LineWrap val)
{
	switch (val) {
	case Settings::LINE_WRAP_NONE:
		return QsciScintilla::WrapNone;
	case Settings::LINE_WRAP_CHARACTER:
		return QsciScintilla::WrapCharacter;
	case Settings::LINE_WRAP_WORD:
		return QsciScintilla::WrapWord;
	default:
		assert(false && "unknown wrap mode setting");
	}
}

QsciScintilla::WrapVisualFlag SettingsConverter::fromLineWrapVisualization(Settings::LineWrapVisualization val)
{
	switch (val) {
	case Settings::LINE_WRAP_VISUALIZATION_NONE:
		return QsciScintilla::WrapFlagNone;
	case Settings::LINE_WRAP_VISUALIZATION_TEXT:
		return QsciScintilla::WrapFlagByText;
	case Settings::LINE_WRAP_VISUALIZATION_BORDER:
		return QsciScintilla::WrapFlagByBorder;
	case Settings::LINE_WRAP_VISUALIZATION_MARGIN:
		return QsciScintilla::WrapFlagInMargin;
	default:
		assert(false && "unknown wrap visualization setting");
	}
}

QsciScintilla::WrapIndentMode SettingsConverter::fromLineWrapIndentationStyle(Settings::LineWrapIndentationStyle val)
{
	switch (val) {
	case Settings::LINE_WRAP_INDENTATION_FIXED:
		return QsciScintilla::WrapIndentFixed;
	case Settings::LINE_WRAP_INDENTATION_SAME:
		return QsciScintilla::WrapIndentSame;
	case Settings::LINE_WRAP_INDENTATION_INDENTED:
		return QsciScintilla::WrapIndentIndented;
	default:
		assert(false && "unknown wrap indentation style setting");
	}
}

QsciScintilla::WhitespaceVisibility SettingsConverter::fromShowWhitespaces(Settings::ShowWhitespaces val)
{
	switch(val) {
	case Settings::SHOW_WHITESPACES_NEVER:
		return QsciScintilla::WsInvisible;
	case Settings::SHOW_WHITESPACES_ALWAYS:
		return QsciScintilla::WsVisible;
	case Settings::SHOW_WHITESPACES_AFTER_INDENTATION:
		return QsciScintilla::WsVisibleAfterIndent;
	default:
		assert(false && "unknown show whitespace setting");
	}
}

EditorColorScheme::EditorColorScheme(fs::path path) : path(path)
{
	try {
		boost::property_tree::read_json(boosty::stringy(path).c_str(), pt);
		_name = QString(pt.get<std::string>("name").c_str());
		_index = pt.get<int>("index");
	} catch (const std::exception & e) {
		PRINTB("Error reading color scheme file '%s': %s", path.c_str() % e.what());
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
	scintillaLayout = new QVBoxLayout(this);
	qsci = new QsciScintilla(this);

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

	scintillaLayout->setContentsMargins(0, 0, 0, 0);
	scintillaLayout->addWidget(qsci);

	qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, indicatorNumber);
	qsci->markerDefine(QsciScintilla::Circle, markerNumber);
	qsci->setUtf8(true);
	applySettings();

	lexer = new ScadLexer(this);
	qsci->setLexer(lexer);
	initMargin();

	connect(qsci, SIGNAL(textChanged()), this, SIGNAL(contentsChanged()));
	connect(qsci, SIGNAL(modificationChanged(bool)), this, SIGNAL(modificationChanged(bool)));
}

/**
 * Apply the settings that are changeable in the preferences. This is also
 * called in the event handler from the preferences.
 */
void ScintillaEditor::applySettings()
{
	SettingsConverter conv;
	Settings::Settings *s = Settings::Settings::inst();

	qsci->setIndentationWidth(s->get(Settings::Settings::indentationWidth));
	qsci->setTabWidth(s->get(Settings::Settings::tabWidth));
	qsci->setWrapMode(conv.fromWrapMode(s->get(Settings::Settings::lineWrap)));
	qsci->setWrapIndentMode(conv.fromLineWrapIndentationStyle(s->get(Settings::Settings::lineWrapIndentationStyle)));
	qsci->setWrapVisualFlags(conv.fromLineWrapVisualization(s->get(Settings::Settings::lineWrapVisualizationEnd)),
		conv.fromLineWrapVisualization(s->get(Settings::Settings::lineWrapVisualizationBegin)),
		s->get(Settings::Settings::lineWrapIndentation));
	qsci->setWhitespaceVisibility(conv.fromShowWhitespaces(s->get(Settings::Settings::showWhitespaces)));
	qsci->setWhitespaceSize(s->get(Settings::Settings::showWhitespacesSize));
	qsci->setAutoIndent(s->get(Settings::Settings::autoIndent));
	qsci->setTabIndents(s->get(Settings::Settings::tabIndents));
	qsci->setIndentationsUseTabs(s->get(Settings::Settings::indentationsUseTabs));

	qsci->setBraceMatching(QsciScintilla::SloppyBraceMatch);
	qsci->setFolding(QsciScintilla::BoxedTreeFoldStyle, 4);
	qsci->setCaretLineVisible(true);
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	qsci->SCN_SAVEPOINTLEFT();
#endif
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
	qsci->fillIndicatorRange(line, index, line, index + 1, indicatorNumber);
	qsci->markerAdd(line, markerNumber);
}

void ScintillaEditor::unhighlightLastError()
{
	int totalLength = qsci->text().length();
	int line, index;
	qsci->lineIndexFromPosition(totalLength, &line, &index);
	qsci->clearIndicatorRange(0, 0, line, index, indicatorNumber);
	qsci->markerDeleteAll(markerNumber);
}

QColor ScintillaEditor::readColor(const boost::property_tree::ptree &pt, const std::string name, const QColor defaultColor)
{
	try {
		const std::string val = pt.get<std::string>(name);
		return QColor(val.c_str());
	} catch (std::exception e) {
		return defaultColor;
	}
}

std::string ScintillaEditor::readString(const boost::property_tree::ptree &pt, const std::string name, const std::string defaultValue)
{
	try {
		const std::string val = pt.get<std::string>(name);
		return val;
	} catch (std::exception e) {
		return defaultValue;
	}
}

int ScintillaEditor::readInt(const boost::property_tree::ptree &pt, const std::string name, const int defaultValue)
{
	try {
		const int val = pt.get<int>(name);
		return val;
	} catch (std::exception e) {
		return defaultValue;
	}
}

void ScintillaEditor::setColormap(const EditorColorScheme *colorScheme)
{
	const boost::property_tree::ptree & pt = colorScheme->propertyTree();

	try {
		QFont font = lexer->font(lexer->defaultStyle());
		const QColor textColor(pt.get<std::string>("text").c_str());
		const QColor paperColor(pt.get<std::string>("paper").c_str());

		ScadLexer *l = new ScadLexer(this);

		// Keywords must be set before the lexer is attached to QScintilla
		// as they seem to be read and cached at attach time.
		boost::optional<const boost::property_tree::ptree&> keywords = pt.get_child_optional("keywords");
		if (keywords.is_initialized()) {
			l->setKeywords(1, readString(keywords.get(), "keyword-set1", ""));
			l->setKeywords(2, readString(keywords.get(), "keyword-set2", ""));
			l->setKeywords(3, readString(keywords.get(), "keyword-set-doc", ""));
			l->setKeywords(4, readString(keywords.get(), "keyword-set3", ""));
		}

		qsci->setLexer(l);
		delete lexer;
		lexer = l;

		// All other properties must be set after attaching to QSCintilla so
		// the editor gets the change events and updates itself to match
		l->setFont(font);
		l->setColor(textColor);
		l->setPaper(paperColor);

		const boost::property_tree::ptree& colors = pt.get_child("colors");
		l->setColor(readColor(colors, "keyword1", textColor), QsciLexerCPP::Keyword);
		l->setColor(readColor(colors, "keyword2", textColor), QsciLexerCPP::KeywordSet2);
		l->setColor(readColor(colors, "keyword3", textColor), QsciLexerCPP::GlobalClass);
		l->setColor(readColor(colors, "number", textColor), QsciLexerCPP::Number);
		l->setColor(readColor(colors, "string", textColor), QsciLexerCPP::SingleQuotedString);
		l->setColor(readColor(colors, "string", textColor), QsciLexerCPP::DoubleQuotedString);
		l->setColor(readColor(colors, "operator", textColor), QsciLexerCPP::Operator);
		l->setColor(readColor(colors, "comment", textColor), QsciLexerCPP::Comment);
		l->setColor(readColor(colors, "commentline", textColor), QsciLexerCPP::CommentLine);
		l->setColor(readColor(colors, "commentdoc", textColor), QsciLexerCPP::CommentDoc);
		l->setColor(readColor(colors, "commentdoc", textColor), QsciLexerCPP::CommentLineDoc);
		l->setColor(readColor(colors, "commentdockeyword", textColor), QsciLexerCPP::CommentDocKeyword);

		const boost::property_tree::ptree& caret = pt.get_child("caret");
		qsci->setCaretWidth(readInt(caret, "width", 1));
		qsci->setCaretForegroundColor(readColor(caret, "foreground", textColor));
		qsci->setCaretLineBackgroundColor(readColor(caret, "line-background", paperColor));

		qsci->setMarkerBackgroundColor(readColor(colors, "error-marker", QColor(255, 0, 0, 100)), markerNumber);
		qsci->setIndicatorForegroundColor(readColor(colors, "error-indicator", QColor(255, 0, 0, 100)), indicatorNumber);
		qsci->setIndicatorOutlineColor(readColor(colors, "error-indicator-outline", QColor(255, 0, 0, 100)), indicatorNumber);
		qsci->setWhitespaceBackgroundColor(readColor(colors, "whitespace-background", paperColor));
		qsci->setWhitespaceForegroundColor(readColor(colors, "whitespace-foreground", textColor));
		qsci->setMarginsBackgroundColor(readColor(colors, "margin-background", paperColor));
		qsci->setMarginsForegroundColor(readColor(colors, "margin-foreground", textColor));
		qsci->setMatchedBraceBackgroundColor(readColor(colors, "matched-brace-background", paperColor));
		qsci->setMatchedBraceForegroundColor(readColor(colors, "matched-brace-foreground", textColor));
		qsci->setUnmatchedBraceBackgroundColor(readColor(colors, "unmatched-brace-background", paperColor));
		qsci->setUnmatchedBraceForegroundColor(readColor(colors, "unmatched-brace-foreground", textColor));
		qsci->setSelectionForegroundColor(readColor(colors, "selection-foreground", paperColor));
		qsci->setSelectionBackgroundColor(readColor(colors, "selection-background", textColor));
		qsci->setFoldMarginColors(readColor(colors, "margin-foreground", textColor),
			readColor(colors, "margin-background", paperColor));
		qsci->setEdgeColor(readColor(colors, "edge", textColor));
	} catch (std::exception e) {
		noColor();
	}
}

void ScintillaEditor::noColor()
{
	lexer->setPaper(Qt::white);
	lexer->setColor(Qt::black);
	qsci->setCaretWidth(2);
	qsci->setCaretForegroundColor(Qt::black);
	qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
	qsci->setIndicatorForegroundColor(QColor(255, 0, 0, 128), indicatorNumber);
	qsci->setIndicatorOutlineColor(QColor(0, 0, 0, 255), indicatorNumber); // only alpha part is used
	qsci->setCaretLineBackgroundColor(Qt::white);
	qsci->setWhitespaceBackgroundColor(Qt::white);
	qsci->setWhitespaceForegroundColor(Qt::black);
	qsci->setMarginsBackgroundColor(Qt::white);
	qsci->setMarginsForegroundColor(Qt::black);
	qsci->setSelectionForegroundColor(Qt::white);
	qsci->setSelectionBackgroundColor(Qt::black);
	qsci->setMatchedBraceBackgroundColor(Qt::white);
	qsci->setMatchedBraceForegroundColor(Qt::black);
	qsci->setUnmatchedBraceBackgroundColor(Qt::white);
	qsci->setUnmatchedBraceForegroundColor(Qt::black);
	qsci->setMarginsBackgroundColor(Qt::lightGray);
	qsci->setMarginsForegroundColor(Qt::black);
	qsci->setFoldMarginColors(Qt::black, Qt::lightGray);
	qsci->setEdgeColor(Qt::black);
}

void ScintillaEditor::enumerateColorSchemesInPath(ScintillaEditor::colorscheme_set_t &result_set, const fs::path path)
{
	const fs::path color_schemes = path / "color-schemes" / "editor";

	fs::directory_iterator end_iter;

	if (fs::exists(color_schemes) && fs::is_directory(color_schemes)) {
		for (fs::directory_iterator dir_iter(color_schemes); dir_iter != end_iter; ++dir_iter) {
			if (!fs::is_regular_file(dir_iter->status())) {
				continue;
			}

			const fs::path path = (*dir_iter).path();
			if (!(path.extension().string() == ".json")) {
				continue;
			}

			EditorColorScheme *colorScheme = new EditorColorScheme(path);
			if (colorScheme->valid()) {
				result_set.insert(colorscheme_set_t::value_type(colorScheme->index(), boost::shared_ptr<EditorColorScheme>(colorScheme)));
			} else {
				delete colorScheme;
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
	const colorscheme_set_t colorscheme_set = enumerateColorSchemes();

	QStringList colorSchemes;
	for (colorscheme_set_t::const_iterator it = colorscheme_set.begin(); it != colorscheme_set.end(); it++) {
		colorSchemes << (*it).second.get()->name();
	}
	colorSchemes << "Off";

	return colorSchemes;
}

void ScintillaEditor::setHighlightScheme(const QString &name)
{
	const colorscheme_set_t colorscheme_set = enumerateColorSchemes();

	for (colorscheme_set_t::const_iterator it = colorscheme_set.begin(); it != colorscheme_set.end(); it++) {
		const EditorColorScheme *colorScheme = (*it).second.get();
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

void ScintillaEditor::replaceAll(const QString &text)
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
	QFont font(fontName, size);
	font.setFixedPitch(true);
	lexer->setFont(font);
}

void ScintillaEditor::initMargin()
{
	QFontMetrics fontmetrics = QFontMetrics(qsci->font());
	qsci->setMarginsFont(qsci->font());
	qsci->setMarginWidth(1, fontmetrics.width(QString::number(qsci->lines())) + 6);
	qsci->setMarginLineNumbers(1, true);

	connect(qsci, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

void ScintillaEditor::onTextChanged()
{
	QFontMetrics fontmetrics = qsci->fontMetrics();
	qsci->setMarginWidth(1, fontmetrics.width(QString::number(qsci->lines())) + 6);
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
	if (qsci->selectedText() != newText) qsci->replaceSelectedText(newText);
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
	for (int line = lineFrom; line <= lineTo; line++) {
		qsci->indent(line);
	}
}

void ScintillaEditor::unindentSelection()
{
	int lineFrom, lineTo;
	getRange(&lineFrom, &lineTo);
	for (int line = lineFrom; line <= lineTo; line++) {
		qsci->unindent(line);
	}
}

void ScintillaEditor::commentSelection()
{
	bool hasSelection = qsci->hasSelectedText();

	int lineFrom, lineTo;
	getRange(&lineFrom, &lineTo);
	for (int line = lineFrom; line <= lineTo; line++) {
		qsci->insertAt("//", line, 0);
	}

	if (hasSelection) {
		qsci->setSelection(lineFrom, 0, lineTo, std::max(0, qsci->lineLength(lineTo) - 1));
	}
}

void ScintillaEditor::uncommentSelection()
{
	bool hasSelection = qsci->hasSelectedText();

	int lineFrom, lineTo;
	getRange(&lineFrom, &lineTo);
	for (int line = lineFrom; line <= lineTo; line++) {
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

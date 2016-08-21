#include <stdlib.h>
#include <algorithm>
#include <QString>
#include <QChar>
#include "scintillaeditor.h"
#include <Qsci/qscicommandset.h>
#include "Preferences.h"
#include "PlatformUtils.h"
#include "settings.h"
#include <boost/filesystem.hpp>
namespace fs=boost::filesystem;

class SettingsConverter {
public:
	QsciScintilla::WrapMode toWrapMode(Value val);
	QsciScintilla::WrapVisualFlag toLineWrapVisualization(Value val);
	QsciScintilla::WrapIndentMode toLineWrapIndentationStyle(Value val);
	QsciScintilla::WhitespaceVisibility toShowWhitespaces(Value val);
};

QsciScintilla::WrapMode SettingsConverter::toWrapMode(Value val)
{
	std::string v = val.toString();
	if (v == "Char") {
		return QsciScintilla::WrapCharacter;
	} else if (v == "Word") {
		return QsciScintilla::WrapWord;
	} else {
		return QsciScintilla::WrapNone;
	}
}

QsciScintilla::WrapVisualFlag SettingsConverter::toLineWrapVisualization(Value val)
{
	std::string v = val.toString();
	if (v == "Text") {
		return QsciScintilla::WrapFlagByText;
	} else if (v == "Border") {
		return QsciScintilla::WrapFlagByBorder;
#if QSCINTILLA_VERSION >= 0x020700
	} else if (v == "Margin") {
		return QsciScintilla::WrapFlagInMargin;
#endif
	} else {
		return QsciScintilla::WrapFlagNone;
	}
}

QsciScintilla::WrapIndentMode SettingsConverter::toLineWrapIndentationStyle(Value val)
{
	std::string v = val.toString();
	if (v == "Same") {
		return QsciScintilla::WrapIndentSame;
	} else if (v == "Indented") {
		return QsciScintilla::WrapIndentIndented;
	} else {
		return QsciScintilla::WrapIndentFixed;
	}
}

QsciScintilla::WhitespaceVisibility SettingsConverter::toShowWhitespaces(Value val)
{
	std::string v = val.toString();
	if (v == "Always") {
		return QsciScintilla::WsVisible;
	} else if (v == "AfterIndentation") {
		return QsciScintilla::WsVisibleAfterIndent;
	} else {
		return QsciScintilla::WsInvisible;
	}
}

EditorColorScheme::EditorColorScheme(fs::path path) : path(path)
{
	try {
		boost::property_tree::read_json(path.generic_string().c_str(), pt);
		_name = QString(pt.get<std::string>("name").c_str());
		_index = pt.get<int>("index");
	} catch (const std::exception & e) {
		PRINTB("Error reading color scheme file '%s': %s", path.generic_string().c_str() % e.what());
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
	c->setAlternateKey(Qt::Key_Y | Qt::CTRL);

	scintillaLayout->setContentsMargins(0, 0, 0, 0);
	scintillaLayout->addWidget(qsci);

	qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, indicatorNumber);
	qsci->markerDefine(QsciScintilla::Circle, markerNumber);
	qsci->setUtf8(true);
	qsci->setFolding(QsciScintilla::BoxedTreeFoldStyle, 4);

	lexer = new ScadLexer(this);
	qsci->setLexer(lexer);
	initMargin();

	connect(qsci, SIGNAL(textChanged()), this, SIGNAL(contentsChanged()));
	connect(qsci, SIGNAL(modificationChanged(bool)), this, SIGNAL(modificationChanged(bool)));
	qsci->installEventFilter(this);
}

/**
 * Apply the settings that are changeable in the preferences. This is also
 * called in the event handler from the preferences.
 */
void ScintillaEditor::applySettings()
{
	SettingsConverter conv;
	Settings::Settings *s = Settings::Settings::inst();

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

	std::string indentStyle = s->get(Settings::Settings::indentStyle).toString();
	qsci->setIndentationsUseTabs(indentStyle == "Tabs");
	std::string tabKeyFunction = s->get(Settings::Settings::tabKeyFunction).toString();
	qsci->setTabIndents(tabKeyFunction == "Indent");

	qsci->setBraceMatching(s->get(Settings::Settings::enableBraceMatching).toBool() ? QsciScintilla::SloppyBraceMatch : QsciScintilla::NoBraceMatch);
	qsci->setCaretLineVisible(s->get(Settings::Settings::highlightCurrentLine).toBool());
    bool value = s->get(Settings::Settings::enableLineNumbers).toBool();
    qsci->setMarginLineNumbers(1,value);

    if(!value)
    {
             qsci->setMarginWidth(1,20);
    }
    else
    {
        qsci->setMarginWidth(1,QString(trunc(log10(qsci->lines())+4), '0'));
    }
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
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
		if ((val.length() == 9) && (val.at(0) == '#')) {
			const std::string rgb = std::string("#") + val.substr(3);
			QColor qcol(rgb.c_str());
			unsigned long alpha = std::strtoul(val.substr(1, 2).c_str(), 0, 16);
			qcol.setAlpha(alpha);
			return qcol;
		}
#endif
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
                // Somehow, the margin font got lost when we deleted the old lexer
                qsci->setMarginsFont(font);

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
	const fs::path color_schemes = path / "color-schemes" / "editor";

	fs::directory_iterator end_iter;

	if (fs::exists(color_schemes) && fs::is_directory(color_schemes)) {
		for (fs::directory_iterator dir_iter(color_schemes); dir_iter != end_iter; ++dir_iter) {
			if (!fs::is_regular_file(dir_iter->status())) {
				continue;
			}

			const fs::path path = (*dir_iter).path();
			if (!(path.extension() == ".json")) {
				continue;
			}

			EditorColorScheme *colorScheme = new EditorColorScheme(path);
			if (colorScheme->valid()) {
				result_set.insert(colorscheme_set_t::value_type(colorScheme->index(), shared_ptr<EditorColorScheme>(colorScheme)));
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

bool ScintillaEditor::canUndo()
{
    return qsci->isUndoAvailable();
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
  lexer->setFont(this->currentFont);
  qsci->setMarginsFont(this->currentFont);
  onTextChanged(); // Update margin width
}

void ScintillaEditor::initMargin()
{
  qsci->setMarginLineNumbers(1, true);
  connect(qsci, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

void ScintillaEditor::onTextChanged()
{
  Settings::Settings *s = Settings::Settings::inst();
  QFontMetrics fontmetrics(this->currentFont);
  bool value = s->get(Settings::Settings::enableLineNumbers).toBool();

  if(!value)
  {
           qsci->setMarginWidth(1,20);
  }
  else
  {
      qsci->setMarginWidth(1,QString(trunc(log10(qsci->lines())+4), '0'));
  }
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
#if QSCINTILLA_VERSION >= 0x020700
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

bool ScintillaEditor::eventFilter(QObject* obj, QEvent *e)
{
	static bool wasChanged=false;
	static bool previewAfterUndo=false;

	if (obj != qsci) return EditorInterface::eventFilter(obj, e);

	if (	e->type()==QEvent::KeyPress
		 || e->type()==QEvent::KeyRelease
		 ) {
		QKeyEvent *ke = static_cast<QKeyEvent*>(e);
		if ((ke->modifiers() & ~Qt::KeypadModifier) == Qt::AltModifier) {
			switch (ke->key())
			{
				case Qt::Key_Left:
				case Qt::Key_Right:
					if (e->type()==QEvent::KeyPress) {
						navigateOnNumber(ke->key());
					}
					return true;

				case Qt::Key_Up:
				case Qt::Key_Down:
					if (e->type()==QEvent::KeyPress) {
						if (!wasChanged) qsci->beginUndoAction();
						if (modifyNumber(ke->key())) {
							wasChanged=true;
							previewAfterUndo=true;
						}
						if (!wasChanged) qsci->endUndoAction();
					}
					return true;
			}
		}
		if (previewAfterUndo && e->type()==QEvent::KeyPress) {
			int k=ke->key() | ke->modifiers();
			if (wasChanged) qsci->endUndoAction();
			wasChanged=false;
			QsciCommand *cmd=qsci->standardCommands()->boundTo(k);
			if ( cmd && ( cmd->command()==QsciCommand::Undo || cmd->command()==QsciCommand::Redo ) )
				QTimer::singleShot(0,this,SIGNAL(previewRequest()));
			else if ( cmd || !ke->text().isEmpty() ) {
				// any insert or command (but not undo/redo) cancels the preview after undo
				previewAfterUndo=false;
			}
		}
	}
	return false;
}


void ScintillaEditor::navigateOnNumber(int key)
{
	int line, index;
	qsci->getCursorPosition(&line, &index);
	QString text=qsci->text(line);
	QString left=text.left(index);
	bool dotOnLeft=left.contains(QRegExp("\\.\\d*$"));
	bool dotJustLeft=index>1 && text[index-2]=='.';
	bool dotJustRight=text[index]=='.';
	bool numOnLeft=left.contains(QRegExp("\\d\\.?$")) || left.endsWith("-.");
	bool numOnRight=text.indexOf(QRegExp("\\.?\\d"),index)==index;

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
	QString text=qsci->text(line);

	int lineFrom, indexFrom, lineTo, indexTo;
	qsci->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
	bool hadSelection=qsci->hasSelectedText();

	int begin=QRegExp("[-+]?\\d*\\.?\\d*$").indexIn(text.left(index));
	int end=text.indexOf(QRegExp("[^0-9.]"),index);
	if (end<0) end=text.length();
	QString nr=text.mid(begin,end-begin);
	if ( !(nr.contains(QRegExp("^[-+]?\\d*\\.?\\d*$")) && nr.contains(QRegExp("\\d"))) ) return false;
	bool sign=nr[0]=='+'||nr[0]=='-';
	if (nr.endsWith('.')) nr=nr.left(nr.length()-1);
	int curpos=index-begin;
	int dotpos=nr.indexOf('.');
	int decimals=dotpos<0?0:nr.length()-dotpos-1;
	long long int number=(dotpos<0)?nr.toLongLong():(nr.left(dotpos)+nr.mid(dotpos+1)).toLongLong();
	int tail=nr.length()-curpos;
	int exponent=tail-((dotpos>=curpos)?1:0);
	long long int step=1;
	for (int i=exponent; i>0; i--) step*=10;

	switch (key) {
		case Qt::Key_Up:   number+=step; break;
		case Qt::Key_Down: number-=step; break;
	}
	bool negative=number<0;
	if (negative) number=-number;
	QString newnr=QString::number(number);
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

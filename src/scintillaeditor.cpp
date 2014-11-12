#include <algorithm>
#include <QString>
#include <QChar>
#include "boosty.h"
#include "scintillaeditor.h"
#include <Qsci/qscicommandset.h>
#include "Preferences.h"
#include "PlatformUtils.h"

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


  //
  // Remapping some scintilla key binding which conflict with OpenSCAD global
  // key bindings, as well as some minor scintilla bugs
  //
  QsciCommand *c;
#ifdef Q_OS_MAC
  // Alt-Backspace should delete left word (Alt-Delete already deletes right word)
  c= qsci->standardCommands()->find(QsciCommand::DeleteWordLeft);
  c->setKey(Qt::Key_Backspace | Qt::ALT);
#endif
  // Cmd/Ctrl-T is handled by the menu
  c = qsci->standardCommands()->boundTo(Qt::Key_T | Qt::CTRL);
  c->setKey(0);
  // Cmd/Ctrl-D is handled by the menu
  c = qsci->standardCommands()->boundTo(Qt::Key_D | Qt::CTRL);
  c->setKey(0);
  // Ctrl-Shift-Z should redo on all platforms
  c= qsci->standardCommands()->find(QsciCommand::Redo);
  c->setKey(Qt::Key_Z | Qt::CTRL | Qt::SHIFT);

  scintillaLayout->setContentsMargins(0, 0, 0, 0);
  scintillaLayout->addWidget(qsci);

  qsci->setBraceMatching (QsciScintilla::SloppyBraceMatch);
  qsci->setWrapMode(QsciScintilla::WrapCharacter);
  qsci->setWrapVisualFlags(QsciScintilla::WrapFlagByBorder, QsciScintilla::WrapFlagNone, 0);
  qsci->setAutoIndent(true);
  qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, indicatorNumber);
  qsci->markerDefine(QsciScintilla::Circle, markerNumber);
  qsci->setUtf8(true);
  qsci->setTabIndents(true);
  qsci->setTabWidth(8);
  qsci->setIndentationWidth(4);
  qsci->setIndentationsUseTabs(false);  
  
  lexer = new ScadLexer(this);
  initLexer();
  initMargin();
  qsci->setFolding(QsciScintilla::BoxedTreeFoldStyle, 4);
  qsci->setCaretLineVisible(true);

  connect(qsci, SIGNAL(textChanged()), this, SIGNAL(contentsChanged()));
  connect(qsci, SIGNAL(modificationChanged(bool)), this, SIGNAL(modificationChanged(bool)));
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
  qsci->fillIndicatorRange(line, index, line, index+1, indicatorNumber);	
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
	const QColor textColor(pt.get<std::string>("text").c_str());
	const QColor paperColor(pt.get<std::string>("paper").c_str());

	lexer->setColor(textColor);
	lexer->setPaper(paperColor);

        const boost::property_tree::ptree& colors = pt.get_child("colors");
	lexer->setColor(readColor(colors, "keyword1", textColor), QsciLexerCPP::Keyword);
	lexer->setColor(readColor(colors, "keyword2", textColor), QsciLexerCPP::KeywordSet2);
	lexer->setColor(readColor(colors, "keyword3", textColor), QsciLexerCPP::GlobalClass);
	lexer->setColor(readColor(colors, "comment", textColor), QsciLexerCPP::CommentDocKeyword);
	lexer->setColor(readColor(colors, "number", textColor), QsciLexerCPP::Number);
	lexer->setColor(readColor(colors, "string", textColor), QsciLexerCPP::DoubleQuotedString);
	lexer->setColor(readColor(colors, "operator", textColor), QsciLexerCPP::Operator);
	lexer->setColor(readColor(colors, "commentline", textColor), QsciLexerCPP::CommentLine);

        const boost::property_tree::ptree& caret = pt.get_child("caret");
	
	qsci->setCaretWidth(readInt(caret, "width", 1));
	qsci->setCaretForegroundColor(readColor(caret, "foreground", textColor));
	qsci->setCaretLineBackgroundColor(readColor(caret, "line-background", paperColor));

	qsci->setMarkerBackgroundColor(readColor(colors, "error-marker", QColor(255, 0, 0, 100)), markerNumber);
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
    qsci->setCaretLineBackgroundColor(Qt::white);
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
    for (colorscheme_set_t::const_iterator it = colorscheme_set.begin();it != colorscheme_set.end();it++) {
        colorSchemes <<  (*it).second.get()->name();
    }
    colorSchemes << "Off";
	
    return colorSchemes;
}

void ScintillaEditor::setHighlightScheme(const QString &name)
{
    const colorscheme_set_t colorscheme_set = enumerateColorSchemes();

    for (colorscheme_set_t::const_iterator it = colorscheme_set.begin();it != colorscheme_set.end();it++) {
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

void ScintillaEditor::initLexer()
{
  qsci->setLexer(lexer);
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
    for (int line = lineFrom;line <= lineTo;line++) {
	qsci->indent(line);
    }
}

void ScintillaEditor::unindentSelection()
{
    int lineFrom, lineTo;
    getRange(&lineFrom, &lineTo);
    for (int line = lineFrom;line <= lineTo;line++) {
	qsci->unindent(line);
    }
}

void ScintillaEditor::commentSelection()
{
    bool hasSelection = qsci->hasSelectedText();
    
    int lineFrom, lineTo;
    getRange(&lineFrom, &lineTo);
    for (int line = lineFrom;line <= lineTo;line++) {
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
    for (int line = lineFrom;line <= lineTo;line++) {
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

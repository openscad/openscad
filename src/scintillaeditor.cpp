#include <algorithm>
#include <QString>
#include <QChar>
#include "scintillaeditor.h"
#include <Qsci/qscicommandset.h>
#include "Preferences.h"
#include "PlatformUtils.h"

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

QColor ScintillaEditor::read_color(boost::property_tree::ptree &pt, const std::string name, const QColor defaultColor)
{
    try {
	const std::string val = pt.get<std::string>(name);
	return QColor(val.c_str());
    } catch (std::exception e) {
	//std::cout << "read_color('" << name << "') failed" << std::endl;
	return defaultColor;
    }
}

int ScintillaEditor::read_int(boost::property_tree::ptree &pt, const std::string name, const int defaultValue)
{
    try {
	const int val = pt.get<int>(name);
	return val;
    } catch (std::exception e) {
	return defaultValue;
    }
}

void ScintillaEditor::read_colormap(const fs::path path)
{
    boost::property_tree::ptree pt;

    try {
	boost::property_tree::read_json(boosty::stringy(path).c_str(), pt);
	const QColor textColor(pt.get<std::string>("text").c_str());
	const QColor paperColor(pt.get<std::string>("paper").c_str());

	lexer->setColor(textColor);
	lexer->setPaper(paperColor);

        boost::property_tree::ptree& colors = pt.get_child("colors");
	lexer->setColor(read_color(colors, "keyword1", textColor), QsciLexerCPP::Keyword);
	lexer->setColor(read_color(colors, "keyword2", textColor), QsciLexerCPP::KeywordSet2);
	lexer->setColor(read_color(colors, "keyword3", textColor), QsciLexerCPP::GlobalClass);
	lexer->setColor(read_color(colors, "comment", textColor), QsciLexerCPP::CommentDocKeyword);
	lexer->setColor(read_color(colors, "number", textColor), QsciLexerCPP::Number);
	lexer->setColor(read_color(colors, "string", textColor), QsciLexerCPP::DoubleQuotedString);
	lexer->setColor(read_color(colors, "operator", textColor), QsciLexerCPP::Operator);
	lexer->setColor(read_color(colors, "commentline", textColor), QsciLexerCPP::CommentLine);

        boost::property_tree::ptree& caret = pt.get_child("caret");
	
	qsci->setCaretWidth(read_int(caret, "width", 1));
	qsci->setCaretForegroundColor(read_color(caret, "foreground", textColor));
	qsci->setCaretLineBackgroundColor(read_color(caret, "line-background", paperColor));

	qsci->setMarkerBackgroundColor(read_color(colors, "error-marker", QColor(255, 0, 0, 100)), markerNumber);
	qsci->setMarginsBackgroundColor(read_color(colors, "margin-background", paperColor));
	qsci->setMarginsForegroundColor(read_color(colors, "margin-foreground", textColor));
	qsci->setMatchedBraceBackgroundColor(read_color(colors, "matched-brace-background", paperColor));
	qsci->setMatchedBraceForegroundColor(read_color(colors, "matched-brace-foreground", textColor));
	qsci->setUnmatchedBraceBackgroundColor(read_color(colors, "unmatched-brace-background", paperColor));
	qsci->setUnmatchedBraceForegroundColor(read_color(colors, "unmatched-brace-foreground", textColor));
	qsci->setSelectionForegroundColor(read_color(colors, "selection-foreground", paperColor));
	qsci->setSelectionBackgroundColor(read_color(colors, "selection-background", textColor));
        qsci->setFoldMarginColors(read_color(colors, "margin-foreground", textColor),
		read_color(colors, "margin-background", paperColor));
	qsci->setEdgeColor(read_color(colors, "edge", textColor));
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

void ScintillaEditor::setHighlightScheme(const QString &name)
{
    fs::path resources = PlatformUtils::resourcesPath();
    fs::path color_schemes = resources / "color-schemes" / "editor";
    
    if (name == "For Light Background") {
	read_colormap(color_schemes / "light-background.json");
    } else if (name == "For Dark Background") {
	read_colormap(color_schemes / "dark-background.json");
    } else if (name == "Monokai") {
	read_colormap(color_schemes / "monokai.json");
    } else if (name == "Solarized") {
	read_colormap(color_schemes / "solarized.json");
    } else if (name == "Off") {
	noColor();
    }
}

void ScintillaEditor::insert(const QString &text)
{
  qsci->insert(text); 
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

void ScintillaEditor::get_range(int *lineFrom, int *lineTo)
{
    int indexFrom, indexTo;
    if (qsci->hasSelectedText()) {
	qsci->getSelection(lineFrom, &indexFrom, lineTo, &indexTo);
    } else {
	qsci->getCursorPosition(lineFrom, &indexFrom);
	*lineTo = *lineFrom;
    }
}

void ScintillaEditor::indentSelection()
{
    int lineFrom, lineTo;
    get_range(&lineFrom, &lineTo);
    for (int line = lineFrom;line <= lineTo;line++) {
	qsci->indent(line);
    }
}

void ScintillaEditor::unindentSelection()
{
    int lineFrom, lineTo;
    get_range(&lineFrom, &lineTo);
    for (int line = lineFrom;line <= lineTo;line++) {
	qsci->unindent(line);
    }
}

void ScintillaEditor::commentSelection()
{
    int lineFrom, lineTo;
    get_range(&lineFrom, &lineTo);
    for (int line = lineFrom;line <= lineTo;line++) {
	qsci->insertAt("//", line, 0);
    }
}

void ScintillaEditor::uncommentSelection()
{
    int lineFrom, lineTo;
    get_range(&lineFrom, &lineTo);
    for (int line = lineFrom;line <= lineTo;line++) {
	QString lineText = qsci->text(line);
	if (lineText.startsWith("//")) {
	    qsci->setSelection(line, 0, line, 2);
	    qsci->removeSelectedText();
	}
    }
}

QString ScintillaEditor::selectedText()
{
  return qsci->selectedText();
}

#include <algorithm>
#include <QString>
#include <QChar>
#include "scintillaeditor.h"
#include <Qsci/qscicommandset.h>
#include "Preferences.h"

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
  this->setHighlightScheme(Preferences::inst()->getValue("editor/syntaxhighlight").toString());

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
#if QSCINTILLA_VERSION >= 0x020800
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
  qsci->setIndicatorForegroundColor(QColor(255,0,0,100));
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

//Editor themes
void ScintillaEditor::forLightBackground()
{
  lexer->setPaper("#fff");
  lexer->setColor(QColor("#272822")); // -> Style: Default text
  lexer->setColor(QColor("Green"), QsciLexerCPP::Keyword);	    // -> Style: Keyword	
  lexer->setColor(QColor("Green"), QsciLexerCPP::KeywordSet2);	    // -> Style: KeywordSet2
  lexer->setColor(Qt::blue, QsciLexerCPP::CommentDocKeyword);	    // -> used in comments only like /*! \cube */
  lexer->setColor(QColor("DarkBlue"), QsciLexerCPP::GlobalClass);	    // -> Style: GlobalClass
  lexer->setColor(Qt::blue, QsciLexerCPP::Operator);
  lexer->setColor(Qt::darkMagenta, QsciLexerCPP::DoubleQuotedString);	
  lexer->setColor(Qt::darkCyan, QsciLexerCPP::Comment);
  lexer->setColor(Qt::darkCyan, QsciLexerCPP::CommentLine);
  lexer->setColor(QColor("DarkRed"), QsciLexerCPP::Number);
  qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
  qsci->setCaretLineBackgroundColor(QColor("#ffe4e4"));
  qsci->setMarginsBackgroundColor(QColor("#ccc"));
  qsci->setMarginsForegroundColor(QColor("#111"));
  qsci->setMatchedBraceBackgroundColor(QColor("#333"));
  qsci->setMatchedBraceForegroundColor(QColor("#fff"));
}

void ScintillaEditor::forDarkBackground()
{
  lexer->setPaper(QColor("#272822"));
  lexer->setColor(QColor(Qt::white));          
  lexer->setColor(QColor("#f12971"), QsciLexerCPP::Keyword);
  lexer->setColor(QColor("#56dbf0"),QsciLexerCPP::KeywordSet2);	
  lexer->setColor(QColor("#ccdf32"), QsciLexerCPP::CommentDocKeyword);
  lexer->setColor(QColor("#56d8f0"), QsciLexerCPP::GlobalClass); 
  lexer->setColor(QColor("#d8d8d8"), QsciLexerCPP::Operator);
  lexer->setColor(QColor("#e6db74"), QsciLexerCPP::DoubleQuotedString);	
  lexer->setColor(QColor("#e6db74"), QsciLexerCPP::CommentLine);
  lexer->setColor(QColor("#af7dff"), QsciLexerCPP::Number);
  qsci->setCaretLineBackgroundColor(QColor(104,225,104, 127));
  qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
  qsci->setMarginsBackgroundColor(QColor("20,20,20,150"));
  qsci->setMarginsForegroundColor(QColor("#fff"));
  qsci->setCaretWidth(2);
  qsci->setCaretForegroundColor(QColor("#ffff00"));
}

void ScintillaEditor::Monokai()
{
  lexer->setPaper("#272822");
  lexer->setColor(QColor("#f8f8f2")); // -> Style: Default text
  lexer->setColor(QColor("#66c3b3"), QsciLexerCPP::Keyword);	    // -> Style: Keyword	
  lexer->setColor(QColor("#79abff"), QsciLexerCPP::KeywordSet2);	    // -> Style: KeywordSet2
  lexer->setColor(QColor("#ccdf32"), QsciLexerCPP::CommentDocKeyword);	    // -> used in comments only like /*! \cube */
  lexer->setColor(QColor("#ffffff"), QsciLexerCPP::GlobalClass);	    // -> Style: GlobalClass
  lexer->setColor(QColor("#d8d8d8"), QsciLexerCPP::Operator);
  lexer->setColor(QColor("#e6db74"), QsciLexerCPP::DoubleQuotedString);	
  lexer->setColor(QColor("#75715e"), QsciLexerCPP::CommentLine);
  lexer->setColor(QColor("#7fb347"), QsciLexerCPP::Number);
  qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
  qsci->setCaretLineBackgroundColor(QColor("#3e3d32"));
  qsci->setMarginsBackgroundColor(QColor("#757575"));
  qsci->setMarginsForegroundColor(QColor("#f8f8f2"));
  qsci->setCaretWidth(2);
  qsci->setCaretForegroundColor(QColor("#ffff00"));
}

void ScintillaEditor::Solarized_light()
{
  lexer->setPaper("#fdf6e3");
  lexer->setColor(QColor("#657b83")); // -> Style: Default text
  lexer->setColor(QColor("#268ad1"), QsciLexerCPP::Keyword);	    // -> Style: Keyword	
  lexer->setColor(QColor("#6c71c4"), QsciLexerCPP::KeywordSet2);	    // -> Style: KeywordSet2
  lexer->setColor(QColor("#b58900"), QsciLexerCPP::CommentDocKeyword);	    // -> used in comments only like /*! \cube */
  lexer->setColor(QColor("#b58800"), QsciLexerCPP::GlobalClass);	    // -> Style: GlobalClass
  lexer->setColor(QColor("#859900"), QsciLexerCPP::Operator);
  lexer->setColor(QColor("#2aa198"), QsciLexerCPP::DoubleQuotedString);	
  lexer->setColor(QColor("#b58800"), QsciLexerCPP::CommentLine);
  lexer->setColor(QColor("#cb4b16"), QsciLexerCPP::Number);
  qsci->setMarkerBackgroundColor(QColor(255, 0, 0, 100), markerNumber);
  qsci->setCaretLineBackgroundColor(QColor("#eeead5"));
  qsci->setMarginsBackgroundColor(QColor("#eee8d5"));
  qsci->setMarginsForegroundColor(QColor("#93a1a1"));
  qsci->setMatchedBraceBackgroundColor(QColor("#0000ff"));
  qsci->setMatchedBraceBackgroundColor(QColor("#333"));
  qsci->setMatchedBraceForegroundColor(QColor("#fff"));
}

void ScintillaEditor::noColor()
{
  lexer->setPaper(Qt::white);
  lexer->setColor(Qt::black);
  qsci->setMarginsBackgroundColor(QColor("#ccc"));
  qsci->setMarginsForegroundColor(QColor("#111"));
	
}

void ScintillaEditor::setHighlightScheme(const QString &name)
{
  if(name == "For Light Background") {
    forLightBackground();
  }
  else if(name == "For Dark Background") {
    forDarkBackground();
  }
  else if(name == "Monokai") {
    Monokai();
  }
  else if(name == "Solarized") {
    Solarized_light();
  }
  else if(name == "Off") {
    noColor();
  }
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

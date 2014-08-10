
#include <iostream>

#include <QChar>
#include <QString>

#include "Preferences.h"
#include "scintillaeditor.h"

ScadApi::ScadApi(QsciScintilla *qsci, QsciLexer *lexer) : QsciAbstractAPIs(lexer), qsci(qsci)
{
	/*
	 * 2d primitives
	 */
	QStringList circle;
	circle
		<< "circle(radius)"
		<< "circle(r = radius)"
		<< "circle(d = diameter)";
	funcs.append(ApiFunc("circle", circle));
	
	QStringList square;
	square
		<< "square(size, center = true)"
		<< "square([width,height], center = true)";
	funcs.append(ApiFunc("square", square));
	
	QStringList polygon;
	polygon
		<< "polygon([points])"
		<< "polygon([points], [paths])";
	funcs.append(ApiFunc("polygon", polygon));
	
	/*
	 * 3d primitives
	 */
	QStringList cube;
	cube
		<< "cube(size)"
		<< "cube([width, depth, height])"
		<< "cube(size = [width, depth, height], center = true)";
	funcs.append(ApiFunc("cube", cube));
	
	QStringList sphere;
	sphere
		<< "sphere(radius)"
		<< "sphere(r = radius)"
		<< "sphere(d = diameter)";
	funcs.append(ApiFunc("sphere", sphere));

	QStringList cylinder;
	cylinder
		<< "cylinder(h, r1, r2)"
		<< "cylinder(h = height, r = radius, center = true)"
		<< "cylinder(h = height, r1 = bottom, r2 = top, center = true)"
		<< "cylinder(h = height, d = diameter, center = true)"
		<< "cylinder(h = height, d1 = bottom, d2 = top, center = true)";
	funcs.append(ApiFunc("cylinder", cylinder));
	
	funcs.append(ApiFunc("polyhedron", "polyhedron(points, triangles, convexity)"));
	
	/*
	 * operations
	 */
	funcs.append(ApiFunc("translate", "translate([x, y, z])"));
	funcs.append(ApiFunc("rotate", "rotate([x, y, z])"));
	funcs.append(ApiFunc("scale", "scale([x, y, z])"));
	funcs.append(ApiFunc("resize", "resize([x, y, z], auto)"));
	funcs.append(ApiFunc("mirror", "mirror([x, y, z])"));
	funcs.append(ApiFunc("multmatrix", "multmatrix(m)"));

	funcs.append(ApiFunc("module", "module"));
	
	funcs.append(ApiFunc("difference", "difference()"));
	funcs.append(ApiFunc("union", "union()"));
	funcs.append(ApiFunc("use", "use"));
	funcs.append(ApiFunc("include", "include"));
	funcs.append(ApiFunc("function", "function"));

	funcs.append(ApiFunc("abs", "abs(number) -> number"));
	funcs.append(ApiFunc("sign", "sign(number) -> -1, 0 or 1"));
	funcs.append(ApiFunc("sin", "sin(degrees) -> number"));
	funcs.append(ApiFunc("cos", "cos(degrees) -> number"));
	funcs.append(ApiFunc("tan", "tan(degrees) -> number"));
	funcs.append(ApiFunc("acos", "acos(number) -> degrees"));
	funcs.append(ApiFunc("asin", "asin(number) -> degrees"));
	funcs.append(ApiFunc("atan", "atan(number) -> degrees"));
	funcs.append(ApiFunc("atan2", "atan2(number, number) -> degrees"));
	funcs.append(ApiFunc("floor", "floor(number) -> number"));
	funcs.append(ApiFunc("round", "round(number) -> number"));
	funcs.append(ApiFunc("ceil", "ceil(number) -> number"));
	funcs.append(ApiFunc("ln", "ln(number) -> number"));
	funcs.append(ApiFunc("len", "len(string) -> number", "len(array) -> number"));
	funcs.append(ApiFunc("log", "log(number) -> number"));
	funcs.append(ApiFunc("pow", "pow(base, exponent) -> number"));
	funcs.append(ApiFunc("sqrt", "sqrt(number) -> number"));
	funcs.append(ApiFunc("exp", "exp(number) -> number"));
	funcs.append(ApiFunc("rands", "rands(min, max, num_results) -> array", "rands(min, max, num_results, seed) -> array"));
	funcs.append(ApiFunc("min", "min(number, number, ...) -> number", "min(array) -> number"));
	funcs.append(ApiFunc("max", "max(number, number, ...) -> number", "max(array) -> number"));
}

ScadApi::~ScadApi()
{
}

void ScadApi::updateAutoCompletionList(const QStringList &context, QStringList &list)
{
	const QString c = context.last();
	for (int a = 0;a < funcs.size();a++) {
		const ApiFunc &func = funcs.at(a);
		const QString &name = func.get_name();
		if (name.startsWith(c)) {
			if (!list.contains(name)) {
				list << name;
			}
		}
	}
}

void ScadApi::autoCompletionSelected (const QString &selection)
{
}

QStringList ScadApi::callTips (const QStringList &context, int commas, QsciScintilla::CallTipsStyle style, QList< int > &shifts)
{
	QStringList callTips;
	for (int a = 0;a < funcs.size();a++) {
		if (funcs.at(a).get_name() == context.at(context.size() - 2)) {
			callTips = funcs.at(a).get_params();
			break;
		}
	}
	return callTips;
}

ScintillaEditor::ScintillaEditor(QWidget *parent) : EditorInterface(parent)
{
	scintillaLayout = new QVBoxLayout(this);
	qsci = new QsciScintilla(this);
	scintillaLayout->setContentsMargins(0, 0, 0, 0);
	scintillaLayout->addWidget(qsci);
	qsci->setBraceMatching (QsciScintilla::SloppyBraceMatch);
	qsci->setWrapMode(QsciScintilla::WrapWord);
	qsci->setWrapVisualFlags(QsciScintilla::WrapFlagByText, QsciScintilla::WrapFlagByText, 0);
	qsci->setAutoIndent(true);
	qsci->indicatorDefine(QsciScintilla::RoundBoxIndicator, indicatorNumber);
	qsci->markerDefine(QsciScintilla::Circle, markerNumber);
	qsci->setUtf8(true);
	preferenceEditorOption = Preferences::inst()->getValue("editor/syntaxhighlight").toString();
	lexer = new ScadLexer(this);
	api = new ScadApi(qsci, lexer);
	initFont();
	initLexer();
	initMargin();
	qsci->setFolding(QsciScintilla::BoxedTreeFoldStyle, 4);
	qsci->setCaretLineVisible(true);
	this->setHighlightScheme(preferenceEditorOption);	

	qsci->setAutoCompletionSource(QsciScintilla::AcsAPIs);
	qsci->setAutoCompletionThreshold(1);
	qsci->setAutoCompletionFillupsEnabled(true);
	qsci->setCallTipsVisible(10);
	qsci->setCallTipsStyle(QsciScintilla::CallTipsContext);
	
	qsci->setTabIndents(true);
	qsci->setTabWidth(8);
	qsci->setIndentationWidth(4);
	qsci->setIndentationsUseTabs(false);
	
	addTemplate("module", "module () {\n    \n}", 7);
	addTemplate("difference", "difference() {\n    union() {\n        \n    }\n}", 37);
	addTemplate("translate", "translate([])", 11);
	addTemplate("rotate", "rotate([])", 8);
	addTemplate("for", "for (i = [  :  ]) {\n    \n}", 11);
	addTemplate("function", "function f(x) = x;", 17);
	
	connect(qsci, SIGNAL(userListActivated(int, const QString &)), this, SLOT(onUserListSelected(const int, const QString &)));
}

void ScintillaEditor::addTemplate(const QString key, const QString text, const int cursor_offset)
{
	templateMap.insert(key, ScadTemplate(text, cursor_offset));
	userList.append(key);
}

void ScintillaEditor::indentSelection()
{
	qsci->showUserList(1, userList);
}
void ScintillaEditor::unindentSelection()
{
	 
}
void ScintillaEditor::commentSelection() 
{

}
void ScintillaEditor::uncommentSelection()
{
	
}
void ScintillaEditor::setPlainText(const QString &text)
{
	qsci->setText(text); 
}

QString ScintillaEditor::toPlainText()
{
	return qsci->text();
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

	if(name == "For Light Background")
	{
		forLightBackground();
	}
	else if(name == "For Dark Background")
	{
		forDarkBackground();
	}
	else if(name == "Monokai")
	{
		Monokai();
	}
	else if(name == "Solarized")
	{
		Solarized_light();
	}
	else if(name == "Off")
	{
		noColor();
	}
}

void ScintillaEditor::insertPlainText(const QString &text)
{
	qsci->setText(text); 
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

void ScintillaEditor::initFont()
{
    QFont font("inconsolata", 12);
    font.setFixedPitch(true);
    qsci->setFont(font);
}

void ScintillaEditor::initLexer()
{
    lexer->setFont(qsci->font());
    qsci->setLexer(lexer);
}

void ScintillaEditor::initMargin()
{
    QFontMetrics fontmetrics = QFontMetrics(qsci->font());
    qsci->setMarginsFont(qsci->font());
    qsci->setMarginWidth(0, fontmetrics.width(QString::number(qsci->lines())) + 6);
    qsci->setMarginLineNumbers(0, true);
 
    connect(qsci, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

void ScintillaEditor::onTextChanged()
{
    QFontMetrics fontmetrics = qsci->fontMetrics();
    qsci->setMarginWidth(0, fontmetrics.width(QString::number(qsci->lines())) + 6);
}

bool ScintillaEditor::findFirst(const QString &expr, bool re, bool cs, bool wo, bool wrap, bool forward, int line, int index, bool show, bool posix)
{
	qsci->findFirst(expr, re, cs, wo, wrap, forward, line, index, show, posix);
}

bool ScintillaEditor::findNext()
{
	qsci->findNext();
}

void ScintillaEditor::replaceSelectedText(QString& newText)
{
	qsci->replaceSelectedText(newText);
}

void ScintillaEditor::onUserListSelected(const int id, const QString &text)
{
	if (!templateMap.contains(text)) {
		return;
	}
	
	ScadTemplate &t = templateMap[text];
	qsci->insert(t.get_text());

	int line, index;
	qsci->getCursorPosition(&line, &index);
	int pos = qsci->positionFromLineIndex(line, index);
	
	pos += t.get_cursor_offset();
	int indent_line = line;
	int indent_width = qsci->indentation(line);
	qsci->lineIndexFromPosition(pos, &line, &index);
	qsci->setCursorPosition(line, index);
	
	int lines = t.get_text().count("\n");
	for (int a = 0;a < lines;a++) {
		qsci->insertAt(QString(" ").repeated(indent_width), indent_line + a + 1, 0);
	}
}

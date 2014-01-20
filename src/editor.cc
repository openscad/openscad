#include "editor.h"
#include "Preferences.h"

Editor::Editor(QWidget *parent) : QTextEdit(parent)
{
	setAcceptRichText(false);

	toolTips["sphere"] = "sphere( radius )";
	toolTips["module"] = "module name(. . .) { . . . }";
	toolTips["function"] = "function name(. . .) = . . .";
	toolTips["include"] = "include <filename.scad>";
	toolTips["use"] = "use <filename.scad>";

	toolTips["circle"] = "circle( radius )";
	toolTips["square"] = "square( [width,height], center=true|false ) or \n"\
		"square( size, center=true|false )";
	toolTips["polygon"] = "polygon(points=[point, point, ...],paths=[[point index, point index, ...],[point index, point index...],...] )";

	toolTips["sphere"] = "sphere( radius )";
	toolTips["cube"] = "cube(size) or cube([width,height,depth])";
	toolTips["cylinder"] = "cylinder(height,radius,center=true|false) or\n"\
		"cylinder(height,radius1,radius2,center=true|false)";
	toolTips["polyhedron"] = "polyhedron(points=[point,point,...],faces=[[pointindex,pointindex,...],[pointindex,pointindex,...],...],convexity)";

	toolTips["translate"] = "translate([x,y,z])";
	toolTips["rotate"] = "rotate([xaxis,yaxis,zaxis])";
	toolTips["scale"] = "scale([x,y,z])";
	toolTips["resize"] = "resize([x,y,z],auto=true|false); or\n"\
		"resize([x,y,z],auto=[true|false,true|false,true|false])";
	toolTips["mirror"] = "mirror([x,y,z])";
	toolTips["color"] = "color([red,green,blue,alpha]) or \n"\
		"color(\"colorname\",alpha)";
	toolTips["hull"] = "hull() { obj1; obj2; ... }";
	toolTips["minkowski"] = "minkowski() { obj1; obj2; ... }";
	toolTips["union"] = "union() { obj1; obj2; ... }";
	toolTips["difference"] = "difference() { obj1; obj2; ... }";
	toolTips["intersection"] = "intersection() { obj1; obj2; ... }";

	toolTips["for"] = "for (i = [start:step:end] { . . do something with i . . . } or \n"\
		"for (i = [a,b,c,d, . . .] { . . do something with i . . . }";
	toolTips["if"] = "if (. . .) { . . . }";
	toolTips["assign"] = "assign (. . .) { . . . }";
	toolTips["import"] = "import(\"filename.stl|.off\",convexity)\n"\
		"for dxf: import(\"filename.dxf\",layername,origin,scale)";
	toolTips["linear_extrude"] = "linear_extrude(height,center=true|false,convexity,twist,slices)";
	toolTips["rotate_extrude"] = "rotate_extrude(convexity)";
	toolTips["projection"] = "projection(cut=true|false)";
	toolTips["surface"] = "surface(file = \"filename.dat\",center,convexity)";

	toolTips["$fa"] = "minimum fragment angle for circle approximations";
	toolTips["$fs"] = "minimum fragment size for circle approximations";
	toolTips["$fn"] = "minimum number of fragments for circle approximations";
	toolTips["$t"] = "animation step";
}

void Editor::indentSelection()
{
	QTextCursor cursor = textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)), QString(QChar(8233)) + QString("\t"));
	if (txt.endsWith(QString(QChar(8233)) + QString("\t")))
		txt.chop(1);
	txt = QString("\t") + txt;

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	setTextCursor(cursor);
}

void Editor::unindentSelection()
{
	QTextCursor cursor = textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("\t"), QString(QChar(8233)));
	if (txt.startsWith(QString("\t")))
		txt.remove(0, 1);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	setTextCursor(cursor);
}

void Editor::commentSelection()
{
	QTextCursor cursor = textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)), QString(QChar(8233)) + QString("//"));
	if (txt.endsWith(QString(QChar(8233)) + QString("//")))
		txt.chop(2);
	txt = QString("//") + txt;

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	setTextCursor(cursor);
}

void Editor::uncommentSelection()
{
	QTextCursor cursor = textCursor();
	int p1 = cursor.selectionStart();
	QString txt = cursor.selectedText();

	txt.replace(QString(QChar(8233)) + QString("//"), QString(QChar(8233)));
	if (txt.startsWith(QString("//")))
		txt.remove(0, 2);

	cursor.insertText(txt);
	int p2 = cursor.position();
	cursor.setPosition(p1, QTextCursor::MoveAnchor);
	cursor.setPosition(p2, QTextCursor::KeepAnchor);
	setTextCursor(cursor);
}

void Editor::zoomIn()
{
	// See also QT's implementation in QEditor.cpp
	QSettings settings;
	QFont tmp_font = this->font() ;
	if ( font().pointSize() >= 1 )
		tmp_font.setPointSize( 1 + font().pointSize() );
	else
		tmp_font.setPointSize( 1 );
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->setFont( tmp_font );
}

void Editor::zoomOut()
{
	QSettings settings;
	QFont tmp_font = this->font();
	if ( font().pointSize() >= 2 )
		tmp_font.setPointSize( -1 + font().pointSize() );
	else
		tmp_font.setPointSize( 1 );
	settings.setValue("editor/fontsize", tmp_font.pointSize());
	this->setFont( tmp_font );
}

void Editor::wheelEvent ( QWheelEvent * event )
{
	if (event->modifiers() == Qt::ControlModifier) {
		if (event->delta() > 0 )
			zoomIn();
		else if (event->delta() < 0 )
			zoomOut();
	} else {
		QTextEdit::wheelEvent( event );
	}
}

bool Editor::event(QEvent* event) {
	if (!Preferences::inst()->getValue("editor/showtooltips").toBool())
		return QTextEdit::event(event);
	if (event->type() == QEvent::ToolTip) {
		QHelpEvent* helpEvent = static_cast <QHelpEvent*>(event);
		QTextCursor cursor = cursorForPosition(helpEvent->pos());
		cursor.select(QTextCursor::WordUnderCursor);
		if (toolTips.contains(cursor.selectedText())) {
			QString tip = toolTips[ cursor.selectedText() ];
			QToolTip::showText( helpEvent->globalPos(), tip );
		} else {
			QToolTip::hideText();
			return true;
		}
	}
	return QTextEdit::event(event);
}

void Editor::setPlainText(const QString &text)
{
	int y = verticalScrollBar()->sliderPosition();
	// Save current cursor position
	QTextCursor cursor = textCursor();
	int n = cursor.position();
	QTextEdit::setPlainText(text);
	// Restore cursor position
	if (n < text.length()) {
		cursor.setPosition(n);
		setTextCursor(cursor);
		verticalScrollBar()->setSliderPosition(y);
	}
}

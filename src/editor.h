#include <QObject>
#include <QString>
#include <QWidget>
#include <QWheelEvent>
#include <QScrollBar>
#include <QTextEdit>
#include "highlighter.h"

class Editor : public QTextEdit
{
	Q_OBJECT
public:
	Editor(QWidget *parent);
	~Editor();
public slots:
	void zoomIn();
	void zoomOut();
	void setLineWrapping(bool on) { if(on) setWordWrapMode(QTextOption::WrapAnywhere); }
	void setContentModified(bool y) { document()->setModified(y); }
	bool isContentModified() { return document()->isModified(); }
	void indentSelection();
	void unindentSelection();
	void commentSelection();
	void uncommentSelection();
	void setPlainText(const QString &text);
	void highlightError(int error_pos);
	void unhighlightLastError();
	void setHighlightScheme(const QString &name);
private:
	void wheelEvent ( QWheelEvent * event );
	Highlighter *highlighter;
};

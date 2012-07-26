#include <QObject>
#include <QString>
#include <QWidget>
#include <QWheelEvent>

#ifdef _QCODE_EDIT_
#include <qeditor.h>
class Editor : public QEditor
#else
#include <QTextEdit>
class Editor : public QTextEdit
#endif
{
	Q_OBJECT
public:
#ifdef _QCODE_EDIT_
	Editor(QWidget *parent) : QEditor(parent) {}
	QString toPlainText() const { return text(); }
	void setPlainText(const QString& text) { setText(text); }
public slots:
	//void zoomIn() { zoom(1); }
	void zoomIn(int n = 1) { zoom(n); }
	//void zoomOut() { zoom(-1); } 
	void zoomOut(int n = 1) { zoom(-n); } 
#else
	Editor(QWidget *parent) : QTextEdit(parent) { setAcceptRichText(false); }
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
private:
	void wheelEvent ( QWheelEvent * event );
#endif
};

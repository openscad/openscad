#include <QObject>
#include <QString>
#include <QWidget>
#include <QWheelEvent>
#include <QScrollBar>
#include <QToolTip>

#include <QTextEdit>
class Editor : public QTextEdit
{
	Q_OBJECT
public:
	Editor(QWidget *parent);
	void setPlainText(const QString &text);
	bool event(QEvent *event);
        QHash<QString, QString> toolTips;
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
};

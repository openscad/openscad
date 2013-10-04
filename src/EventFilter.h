#ifndef FILTER_H_
#define FILTER_H_

#include <QObject>
#include <QFileOpenEvent>
#include "MainWindow.h"

class EventFilter : public QObject
{
	Q_OBJECT;
	
public:
	EventFilter(QObject *parent) : QObject(parent) {}
protected:
	bool eventFilter(QObject *obj, QEvent *event) {
		// Handle Apple event for opening files
		if (event->type() == QEvent::FileOpen) {
			QFileOpenEvent *foe = static_cast<QFileOpenEvent *>(event);
			MainWindow::requestOpenFile(foe->file());
			return true;
		} else {
			// standard event processing
			return QObject::eventFilter(obj, event);
		}
	}
};

#endif

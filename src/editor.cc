#include "editor.h"
#include "Preferences.h"

void EditorInterface::wheelEvent(QWheelEvent *event)
{
	QSettings settings;
	auto wheelzoom_enabled = Preferences::inst()->getValue("editor/ctrlmousewheelzoom").toBool();
	if ((event->modifiers() == Qt::ControlModifier) && wheelzoom_enabled) {
		if (event->delta() > 0) zoomIn();
		else if (event->delta() < 0) zoomOut();
	} else {
		QWidget::wheelEvent(event);
	}
}

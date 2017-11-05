#include "editor.h"
#include "Preferences.h"
#include "QSettingsCached.h"

void EditorInterface::wheelEvent(QWheelEvent *event)
{
	QSettingsCached settings;
	bool wheelzoom_enabled = Preferences::inst()->getValue("editor/ctrlmousewheelzoom").toBool();
	if ((event->modifiers() == Qt::ControlModifier) && wheelzoom_enabled) {
		if (event->delta() > 0) zoomIn();
		else if (event->delta() < 0) zoomOut();
	}
	else {
		QWidget::wheelEvent(event);
	}
}

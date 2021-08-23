#include "Editor.h"
#include "Preferences.h"
#include "QSettingsCached.h"

void EditorInterface::wheelEvent(QWheelEvent *event)
{
	QSettingsCached settings;
	bool wheelzoom_enabled = Preferences::inst()->getValue("editor/ctrlmousewheelzoom").toBool();
	if ((event->modifiers() == Qt::ControlModifier) && wheelzoom_enabled) {
		if (event->angleDelta().x() > 0) zoomIn();
		else if (event->angleDelta().x() < 0) zoomOut();
	} else {
		QWidget::wheelEvent(event);
	}
}

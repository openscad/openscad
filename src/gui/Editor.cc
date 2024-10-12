#include "gui/Editor.h"
#include <QWheelEvent>
#include <QWidget>
#include "gui/Preferences.h"
#include "gui/QSettingsCached.h"

void EditorInterface::wheelEvent(QWheelEvent *event)
{
  QSettingsCached settings;
  bool wheelzoom_enabled = Preferences::inst()->getValue("editor/ctrlmousewheelzoom").toBool();
  if ((event->modifiers() == Qt::ControlModifier) && wheelzoom_enabled) {
    if (event->angleDelta().y() > 0) zoomIn();
    else if (event->angleDelta().y() < 0) zoomOut();
  } else {
    QWidget::wheelEvent(event);
  }
}

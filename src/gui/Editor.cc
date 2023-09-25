#include "Editor.h"
#include "Preferences.h"
#include "QSettingsCached.h"
#include "LanguageRegistry.h"

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

QString EditorInterface::getFileExtension()
{
  return languageRuntime->getFileExtension();
}

QString EditorInterface::getCommentString()
{
  if(commentString.isEmpty())
    commentString = languageRuntime->getCommentString();
  return commentString;
}

void EditorInterface::setLanguageRuntime(std::string suffix)
{
  languageRuntime = 
    LanguageRegistry::instance()->getRuntimeForFileSuffix(suffix);
}

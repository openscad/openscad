#include "UXTest.h"

#include <QString>

#include "platform/PlatformUtils.h"

// NEEDSWORK convert to fs::path
QString UXTest::fixturePath(const QString& relative)
{
  return QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/data/" + relative;
}

void UXTest::setWindow(MainWindow *window_)
{
  window = window_;
}

void UXTest::restoreWindowInitialState()
{
  QString filename = fixturePath("basic-ux/default.scad");
  window->tabManager->open(filename);

  while (window->tabCount > 1) {
    window->tabManager->closeCurrentTab();
  }

  window->designActionAutoReload->setChecked(true);  // Enable auto-reload  & preview
}

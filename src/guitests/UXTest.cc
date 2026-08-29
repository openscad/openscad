#include "UXTest.h"

#include <QString>
#include <QSignalBlocker>

#include "platform/PlatformUtils.h"

void UXTest::setWindow(MainWindow *window_)
{
  window = window_;
}

void UXTest::restoreWindowInitialState()
{
  window->rootGeom.reset();
  window->previewRenderer.reset();
  window->thrownTogetherRenderer.reset();

  QString filename =
    QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/basic-ux/default.scad";
  window->tabManager->open(filename);

  while (window->tabCount > 1) {
    window->tabManager->closeCurrentTab();
  }

  const QSignalBlocker blocker(window->designActionAutoReload);
  window->designActionAutoReload->setChecked(true);  // Enable auto-reload & preview for this test only.
}

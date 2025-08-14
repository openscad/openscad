#include "UXTest.h"
#include "platform/PlatformUtils.h"

void UXTest::setWindow(MainWindow *window_) { window = window_; }

void UXTest::restoreWindowInitialState()
{
  QString filename =
    QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/basic-ux/default.scad";
  window->tabManager->open(filename);

  while (window->tabCount > 1) {
    window->tabManager->closeCurrentTab();
  }

  window->designActionAutoReload->setChecked(true);  // Enable auto-reload  & preview
}

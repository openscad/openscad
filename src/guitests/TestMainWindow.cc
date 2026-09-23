#include "TestMainWindow.h"

#include <QString>
#include <QStringList>
#include <QTest>

#include "gui/OpenSCADApp.h"
#include "platform/PlatformUtils.h"

void TestMainWindow::checkOpenTabPropagateToWindow()
{
  restoreWindowInitialState();

  QString filename =
    QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/basic-ux/empty.scad";

  // When we open a new file,
  window->tabManager->open(filename);

  // The window title must also have the name of open file
  QCOMPARE(window->windowTitle(), QFileInfo(filename).fileName());

  filename = QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/basic-ux/empty2.scad";

  // When we open a new file,
  window->tabManager->open(filename);

  // The window title must also have the name of open file
  QCOMPARE(window->windowTitle(), QFileInfo(filename).fileName());
}

void TestMainWindow::checkSaveToShouldUpdateWindowTitle()
{
  restoreWindowInitialState();

  QString filename =
    QString::fromStdString(PlatformUtils::resourceBasePath()) + "/tests/basic-ux/empty.scad";

  // When we open a new file,
  window->tabManager->open(filename);

  window->tabManager->saveAs(window->activeEditor, "test-tmp.scad");

  // The window title must also have the name of open file
  QCOMPARE(window->windowTitle(), "test-tmp.scad");
}

void TestMainWindow::checkClosingWindowDoesNotUseFreedMembers()
{
  restoreWindowInitialState();

  const int windowCountBefore = scadApp->windowManager.getWindows().size();

  auto *extraWindow = new MainWindow(QStringList());
  QCOMPARE(scadApp->windowManager.getWindows().size(), windowCountBefore + 1);

  // Closing a window destroys its members, and destroying a child widget delivers events while
  // that is happening. MainWindow is still installed as an event filter at that point, so
  // MainWindow::eventFilter() can run against members that have already been destroyed and read
  // freed memory. That is silent in an ordinary build and a heap-use-after-free under
  // AddressSanitizer, which is what this test exists to catch.
  extraWindow->close();
  QCoreApplication::processEvents();

  QCOMPARE(scadApp->windowManager.getWindows().size(), windowCountBefore);
}

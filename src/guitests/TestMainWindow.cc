#include "TestMainWindow.h"

#include <QSet>
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

void TestMainWindow::checkOpeningWindowDuringTestRunDoesNotCrash()
{
  restoreWindowInitialState();

  const int windowCountBefore = scadApp->windowManager.getWindows().size();

  // This test runs inside the same loop (openscad_gui.cc) that iterates
  // app.windowManager.getWindows() to dispatch tests to every open window. Opening a window here,
  // exactly as "File > New Window" (MainWindow::on_fileActionNewWindow_triggered(), a private
  // slot) would, grows that QSet while the loop may still be iterating it -- which crashes
  // unless that loop first takes a snapshot of the window set.
  new MainWindow(QStringList());

  const auto windows = scadApp->windowManager.getWindows();
  QCOMPARE(windows.size(), windowCountBefore + 1);

  // Clean up the extra window so later tests see the expected single-window state.
  for (auto *w : windows) {
    if (w != window) {
      w->close();
      break;
    }
  }
}

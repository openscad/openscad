#include "TestMainWindow.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QMainWindow>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTest>

#include "gui/ColorLabel.h"
#include "gui/ColorList.h"
#include "gui/Dock.h"
#include "platform/PlatformUtils.h"

void TestMainWindow::checkOpenTabPropagateToWindow()
{
  restoreWindowInitialState();

  QString filename = fixturePath("basic-ux/empty.scad");

  // When we open a new file,
  window->tabManager->open(filename);

  // The window title must also have the name of open file
  QCOMPARE(window->windowTitle(), QFileInfo(filename).fileName());

  filename = fixturePath("basic-ux/empty2.scad");

  // When we open a new file,
  window->tabManager->open(filename);

  // The window title must also have the name of open file
  QCOMPARE(window->windowTitle(), QFileInfo(filename).fileName());
}

void TestMainWindow::checkSaveToShouldUpdateWindowTitle()
{
  restoreWindowInitialState();

  QString filename = fixturePath("basic-ux/empty.scad");

  // When we open a new file,
  window->tabManager->open(filename);

  window->tabManager->saveAs(window->activeEditor, "test-tmp.scad");

  // The window title must also have the name of open file
  QCOMPARE(window->windowTitle(), "test-tmp.scad");
}

void TestMainWindow::copyColorName_data()
{
  QTest::addColumn<QString>("caption");
  QTest::addColumn<QString>("name");
  QTest::newRow("XKCD") << QString("adobe") << QString("xkcd:adobe");
  QTest::newRow("web") << QString("aliceblue") << QString("aliceblue");
}

void TestMainWindow::copyColorName()
{
  QFETCH(QString, caption);
  QFETCH(QString, name);

  auto *list = window->colorListWidget;
  auto *swatch = list->findChild<ColorLabel *>(caption);
  auto *selectedName = list->findChild<QLineEdit *>("lineEditColorNameSelected");
  auto *copy = list->findChild<QAction *>("actionCopyColorName");
  QVERIFY(swatch && selectedName && copy);

  QTest::mouseClick(swatch, Qt::LeftButton);
  QCOMPARE(swatch->text(), caption);
  QCOMPARE(selectedName->text(), name);
  copy->trigger();
  QCOMPARE(QApplication::clipboard()->text(), name);
}

void TestMainWindow::restoreFloatingDockVisibility_data()
{
  QTest::addColumn<bool>("visible");
  QTest::newRow("closed") << false;
  QTest::newRow("open") << true;
}

void TestMainWindow::restoreFloatingDockVisibility()
{
  QFETCH(bool, visible);
  QMainWindow original;
  Dock dock(&original);
  dock.setObjectName("testDock");
  original.addDockWidget(Qt::LeftDockWidgetArea, &dock);
  original.show();
  dock.setFloating(true);
  QVERIFY(dock.isVisible());
  if (!visible) QVERIFY(dock.close());
  const auto state = original.saveState();

  QMainWindow restored;
  Dock restoredDock(&restored);
  restoredDock.setObjectName(dock.objectName());
  restoredDock.setName("Console");
  restoredDock.setNameSuffix("part.scad");
  restored.addDockWidget(Qt::LeftDockWidgetArea, &restoredDock);
  QSignalSpy visibility(&restoredDock, &QDockWidget::visibilityChanged);
  QVERIFY(restored.restoreState(state));
  restored.show();

  QVERIFY(restoredDock.isFloating());
  QCOMPARE(restoredDock.windowTitle(), "Console (part.scad)");
  QCOMPARE(restoredDock.isVisible(), visible);
  QCOMPARE(visibility.contains({true}), visible);
  if (!visible) {
    restoredDock.toggleViewAction()->trigger();
    QVERIFY(restoredDock.isVisible());
    QVERIFY(restoredDock.isFloating());
  }
}

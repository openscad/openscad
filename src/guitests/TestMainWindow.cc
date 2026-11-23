#include "TestMainWindow.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QTest>

#include "gui/ColorLabel.h"
#include "gui/ColorList.h"
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

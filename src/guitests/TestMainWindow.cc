#include "TestMainWindow.h"

#include <QString>
#include <QStringList>
#include <QTest>

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

void TestMainWindow::checkChangingColorSchemeRecolorsPreparedPreview()
{
#ifdef ENABLE_OPENCSG
  restoreWindowInitialState();
  window->show();
  QVERIFY(QTest::qWaitForWindowExposed(window));
  window->qglview->setColorScheme("Cornfield");
  window->activeEditor->setPlainText("cube(100, center = true);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 10000);
  window->qglview->repaint();
  const auto before = window->qglview->grabFramebuffer();
  QVERIFY(!before.isNull());
  const auto cornfield = before.pixelColor(before.width() / 2, before.height() / 2);

  window->qglview->setColorScheme("Starnight");
  window->qglview->repaint();
  const auto after = window->qglview->grabFramebuffer();
  QVERIFY(!after.isNull());
  const auto starnight = after.pixelColor(after.width() / 2, after.height() / 2);

  QVERIFY2(cornfield != starnight,
           "Changing schemes left the prepared preview colored by the previous scheme");
#endif
}

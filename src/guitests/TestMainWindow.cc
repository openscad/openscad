#include "TestMainWindow.h"

#include <QFile>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include "gui/QGLView.h"
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

void TestMainWindow::checkPreviewFollowsActiveTab()
{
  restoreWindowInitialState();

  QTemporaryDir models;
  QVERIFY(models.isValid());
  const auto cube = models.filePath("cube.scad");
  const auto sphere = models.filePath("sphere.scad");
  const auto writeModel = [](const QString& filename, const QByteArray& model) {
    QFile file(filename);
    return file.open(QIODevice::WriteOnly) && file.write(model) == model.size();
  };
  QVERIFY(writeModel(cube, "cube(10);\n"));
  QVERIFY(writeModel(sphere, "include <shape.scad>\n"));
  QVERIFY(writeModel(models.filePath("shape.scad"), "sphere(d=30, $fn=48);\n"));
  const auto reused = models.filePath("reused.scad");
  QVERIFY(writeModel(reused, "cube(20);\n"));

  const auto width = [this]() {
    const auto *renderer = window->qglview->getRenderer();
    return renderer ? renderer->getBoundingBox().sizes().x() : -1.0;
  };
  const auto sphereVisible = [&width]() { return width() > 29.0 && width() < 31.0; };

  window->tabManager->open(cube);
  window->actionRenderPreview();
  QTRY_COMPARE_WITH_TIMEOUT(width(), 10.0, 4000);

  window->tabManager->open(sphere);
  QCOMPARE(window->activeEditor->filepath, sphere);
  QTRY_VERIFY_WITH_TIMEOUT(sphereVisible(), 4000);

  window->designActionRender->trigger();
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom, 4000);

  window->tabManager->prevTab();
  QCOMPARE(window->activeEditor->filepath, cube);
  QTRY_COMPARE_WITH_TIMEOUT(width(), 10.0, 4000);
  QVERIFY(!window->rootGeom);

  window->tabManager->nextTab();
  QCOMPARE(window->activeEditor->filepath, sphere);
  QTRY_VERIFY_WITH_TIMEOUT(sphereVisible(), 4000);

  // Switch away while the include reload is waiting for dependencies.
  window->tabManager->prevTab();
  QTRY_COMPARE_WITH_TIMEOUT(width(), 10.0, 4000);
  window->tabManager->nextTab();
  QTRY_VERIFY_WITH_TIMEOUT(window->waitAfterReloadTimer->isActive(), 4000);
  window->tabManager->prevTab();
  QTRY_VERIFY_WITH_TIMEOUT(!GuiLocker::isLocked(), 4000);
  QCOMPARE(width(), 10.0);

  window->tabManager->actionNew();
  auto *blankEditor = window->activeEditor;
  QVERIFY(blankEditor->filepath.isEmpty());
  QTRY_VERIFY_WITH_TIMEOUT(width() <= 0.0, 4000);

  window->tabManager->open(reused);
  QCOMPARE(window->activeEditor, blankEditor);
  QTRY_COMPARE_WITH_TIMEOUT(width(), 20.0, 4000);

  restoreWindowInitialState();
}

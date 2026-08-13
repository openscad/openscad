#include "TestMainWindow.h"

#include <QElapsedTimer>
#include <QDoubleSpinBox>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <QTest>
#include <QSignalSpy>
#include <QTimer>

#include "gui/OpenSCADApp.h"
#include "gui/Console.h"
#include "gui/ComputeWorker.h"
#include "gui/ProgressWidget.h"
#include "gui/parameter/ParameterWidget.h"
#include "glview/ColorMap.h"
#include "glview/Renderer.h"
#include "openscad.h"
#include "platform/PlatformUtils.h"
#include "Feature.h"
#ifdef ENABLE_PYTHON
#include "python/python_public.h"
#endif

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

void TestMainWindow::checkOpeningLargeFileDoesNotParseInGui()
{
  restoreWindowInitialState();

  QTemporaryFile file(QDir::tempPath() + "/openscad-large-XXXXXX.scad");
  QVERIFY(file.open());
  QByteArray source;
  source.reserve(900000);
  for (int i = 0; i < 100000; ++i) source.append("cube(1);\n");
  QCOMPARE(file.write(source), source.size());
  QVERIFY(file.flush());

  QElapsedTimer dispatch;
  dispatch.start();
  window->tabManager->open(file.fileName());
  QVERIFY2(dispatch.elapsed() < 250, "Opening a file parsed source in the GUI process");

  QCoreApplication::processEvents();
  if (auto *progress = window->findChild<ProgressWidget *>()) {
    const auto worker = window->computeWorkerProcessId();
    progress->cancel();
    QTRY_VERIFY_WITH_TIMEOUT(window->computeWorkerProcessId() != worker, 5000);
  }
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

void TestMainWindow::checkEachWindowHasAComputeWorker()
{
  QTRY_VERIFY_WITH_TIMEOUT(window->computeWorkerProcessId() > 0, 5000);
  const auto firstWorker = window->computeWorkerProcessId();

  QVERIFY(QMetaObject::invokeMethod(window, "on_fileActionNewWindow_triggered"));
  QCOMPARE(scadApp->windowManager.getWindows().size(), 2);

  for (auto *candidate : scadApp->windowManager.getWindows()) {
    if (candidate == window) continue;
    QVERIFY(candidate->computeWorkerProcessId() > 0);
    QVERIFY(candidate->computeWorkerProcessId() != firstWorker);
    candidate->close();
  }
}

void TestMainWindow::checkIsolatedWindowsCanPreviewConcurrently()
{
  restoreWindowInitialState();
  window->activeEditor->setPlainText("for (i = [0:100000]) translate([i, 0, 0]) cube(1);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QVERIFY(window->findChild<ProgressWidget *>() != nullptr);

  auto *other = new MainWindow({});
  other->activeEditor->setPlainText("cube(1);");
  QVERIFY(QMetaObject::invokeMethod(other, "on_designActionPreview_triggered"));
  QVERIFY2(other->findChild<ProgressWidget *>() != nullptr,
           "A busy isolated window blocked preview in another window");

  window->findChild<ProgressWidget *>()->cancel();
  other->findChild<ProgressWidget *>()->cancel();
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QTRY_VERIFY_WITH_TIMEOUT(other->findChild<ProgressWidget *>() == nullptr, 5000);
  other->close();
}

void TestMainWindow::checkWorkerMessageSeverity()
{
  restoreWindowInitialState();
  window->console->clear();
  window->activeEditor->setPlainText(
    "echo(\"ordinary\");\ninclude <definitely-missing.scad>\nassert(false, \"failure\");");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QTRY_VERIFY_WITH_TIMEOUT(window->console->toPlainText().contains("ordinary"), 5000);
  QVERIFY(window->console->toPlainText().contains("definitely-missing.scad"));
  QVERIFY(window->console->toPlainText().contains("failure"));
  QCOMPARE(window->compilationWarningCount(), 1);
  QCOMPARE(window->compilationErrorCount(), 1);
}

void TestMainWindow::checkProcessIsolationRequiresRestart()
{
  const auto existingWorker = window->computeWorkerProcessId();
  Feature::enable_feature(Feature::ExperimentalProcessIsolation.get_name(), false);
  QCOMPARE(window->computeWorkerProcessId(), existingWorker);
  QVERIFY(QMetaObject::invokeMethod(window, "on_fileActionNewWindow_triggered"));
  QCOMPARE(scadApp->windowManager.getWindows().size(), 2);

  for (auto *candidate : scadApp->windowManager.getWindows()) {
    if (candidate == window) continue;
    QVERIFY(candidate->computeWorkerProcessId() > 0);
    QVERIFY(candidate->computeWorkerProcessId() != existingWorker);
    candidate->close();
  }
  Feature::enable_feature(Feature::ExperimentalProcessIsolation.get_name());
}

void TestMainWindow::checkLegacyModeRendersWithoutComputeWorker()
{
  MainWindow::setProcessIsolation(false);
  auto *legacyWindow = new MainWindow({});
  QCOMPARE(legacyWindow->computeWorkerProcessId(), 0);
  legacyWindow->activeEditor->setPlainText("cube(1);");
  const auto invoked = QMetaObject::invokeMethod(legacyWindow, "on_designActionRender_triggered");
  QElapsedTimer timer;
  timer.start();
  while (legacyWindow->rootGeom == nullptr && timer.elapsed() < 30000) QTest::qWait(50);
  const auto rendered = legacyWindow->rootGeom != nullptr;
  legacyWindow->close();
  MainWindow::setProcessIsolation(true);
  QVERIFY(invoked);
  QVERIFY(rendered);
}

void TestMainWindow::checkUnavailableComputeWorkerDoesNotBlockOrRespawnForever()
{
  QElapsedTimer elapsed;
  elapsed.start();
  ComputeWorker unavailableWorker("/definitely/missing/openscad");
  QVERIFY2(elapsed.elapsed() < 250, "Starting a compute worker blocked the GUI thread");
  QSignalSpy diagnostics(&unavailableWorker, &ComputeWorker::diagnostic);
  QTest::qWait(1000);
  QCOMPARE(unavailableWorker.processId(), 0);
  QVERIFY(diagnostics.size() <= 1);
}

void TestMainWindow::checkF6UsesComputeWorkerResult()
{
  restoreWindowInitialState();
  window->activeEditor->setPlainText("cube(1);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 10000);
  QCOMPARE(window->rootGeom->getDimension(), 3u);
}

void TestMainWindow::checkF6UsesCustomizerValues()
{
  restoreWindowInitialState();
  window->activeEditor->setPlainText("size = 1; // [1:10]\ncube(size);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 10000);
  auto *spinBox = window->activeEditor->parameterWidget->findChild<QDoubleSpinBox *>("doubleSpinBox");
  QVERIFY(spinBox != nullptr);
  spinBox->setValue(7);

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 10000);
  QCOMPARE(window->rootGeom->getBoundingBox().max().x(), 7.0);
  window->parseTopLevelDocument();  // Restore parser state for the following ModuleCache suite.
}

void TestMainWindow::checkCancelRespawnsWorkerAndPreservesEditor()
{
  restoreWindowInitialState();
  const QString source = "for (i = [0:100000]) translate([i, 0, 0]) cube(1);";
  window->activeEditor->setPlainText(source);
  const auto worker = window->computeWorkerProcessId();

  QElapsedTimer dispatch;
  dispatch.start();
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QVERIFY2(dispatch.elapsed() < 250, "F6 parsed or evaluated source in the GUI process");
  auto *progress = window->findChild<ProgressWidget *>();
  QVERIFY(progress != nullptr);
  progress->cancel();

  QTRY_VERIFY_WITH_TIMEOUT(window->computeWorkerProcessId() != worker, 5000);
  QCOMPARE(window->activeEditor->toPlainText(), source);
}

void TestMainWindow::checkCooperativeCancelKeepsWorker()
{
  restoreWindowInitialState();
  window->activeEditor->setPlainText("sphere(1, $fn=31);");
  const auto worker = window->computeWorkerProcessId();

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  auto *progress = window->findChild<ProgressWidget *>();
  QVERIFY(progress != nullptr);
  progress->cancel();

  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QCOMPARE(window->computeWorkerProcessId(), worker);
}

void TestMainWindow::checkCrashedWorkerRespawns()
{
  restoreWindowInitialState();
  const auto worker = window->computeWorkerProcessId();
  QVERIFY(worker > 0);
  window->exitComputeWorkerForTest();
  QTRY_VERIFY_WITH_TIMEOUT(
    window->computeWorkerProcessId() > 0 && window->computeWorkerProcessId() != worker, 5000);
}

void TestMainWindow::checkWorkerErrorDoesNotMarkSourceRendered()
{
  restoreWindowInitialState();
  window->console->clear();
  const auto documentName = window->activeEditor->filepath.isEmpty()
                              ? QString("Untitled.scad")
                              : QFileInfo(window->activeEditor->filepath).fileName();
  window->activeEditor->setPlainText("cube(");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QVERIFY(!window->activeEditor->contentsRendered);
  QTRY_VERIFY_WITH_TIMEOUT(window->console->toPlainText().contains("Parser error"), 5000);
  QVERIFY(window->console->toPlainText().contains(documentName));
  QVERIFY(!window->console->toPlainText().contains(".openscad-worker-"));

  window->activeEditor->setPlainText("cube(1);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 5000);
}

void TestMainWindow::checkPreviewDispatchDoesNotBlockGui()
{
  restoreWindowInitialState();
  window->activeEditor->setPlainText("for (i = [0:100000]) translate([i, 0, 0]) cube(1);");
  const auto worker = window->computeWorkerProcessId();

  QElapsedTimer dispatch;
  dispatch.start();
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QVERIFY2(dispatch.elapsed() < 250, "F5 parsed or evaluated source in the GUI process");

  auto *progress = window->findChild<ProgressWidget *>();
  QVERIFY(progress != nullptr);
  progress->cancel();
  QTRY_VERIFY_WITH_TIMEOUT(window->computeWorkerProcessId() != worker, 5000);
}

void TestMainWindow::checkOpenCSGPreparationCanBeCanceled()
{
#ifdef ENABLE_OPENCSG
  restoreWindowInitialState();
  window->activeEditor->setPlainText("for (i = [0:999]) translate([i, 0, 0]) cube(1);");

  QTimer cancelWhenPreparing;
  cancelWhenPreparing.setInterval(1);
  connect(&cancelWhenPreparing, &QTimer::timeout, window, [this, &cancelWhenPreparing]() {
    auto *progress = window->findChild<ProgressWidget *>();
    if (window->previewRenderer && progress) {
      cancelWhenPreparing.stop();
      progress->cancel();
    }
  });
  cancelWhenPreparing.start();

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(!cancelWhenPreparing.isActive(), 10000);
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QVERIFY(window->previewRenderer == nullptr);
#endif
}

void TestMainWindow::checkPreviewDrawsAfterCanceledOpenCSGPreparation()
{
#ifdef ENABLE_OPENCSG
  restoreWindowInitialState();
  window->activeEditor->setPlainText("cube(1);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 10000);
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 10000);
  QTRY_VERIFY_WITH_TIMEOUT(window->qglview->getRenderer() == window->previewRenderer.get(), 5000);
#endif
}

void TestMainWindow::checkOpenCSGPreparationUsesViewportColorScheme()
{
#ifdef ENABLE_OPENCSG
  restoreWindowInitialState();
  window->setColorScheme("Starnight");
  window->activeEditor->setPlainText("cube(1);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 10000);

  Color4f actual;
  QVERIFY(window->previewRenderer->getColorSchemeColor(Renderer::ColorMode::MATERIAL, actual));
  const auto scheme = ColorMap::instance().findColorScheme("Starnight");
  QVERIFY(scheme != nullptr);
  QCOMPARE(actual, ColorMap::getColor(*scheme, RenderColor::OPENCSG_FACE_FRONT_COLOR));
#endif
}

void TestMainWindow::checkF5UsesComputeWorkerResult()
{
  restoreWindowInitialState();
  window->activeEditor->setPlainText("#cube(1);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
#ifdef ENABLE_OPENCSG
  QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 10000);
#else
  QTRY_VERIFY_WITH_TIMEOUT(window->thrownTogetherRenderer != nullptr, 10000);
#endif
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 10000);
}

void TestMainWindow::checkReloadPreviewDispatchDoesNotBlockGui()
{
  restoreWindowInitialState();
  window->activeEditor->setPlainText("for (i = [0:100000]) translate([i, 0, 0]) cube(1);");
  window->lastCompiledDoc.clear();
  const auto worker = window->computeWorkerProcessId();

  QElapsedTimer dispatch;
  dispatch.start();
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionReloadAndPreview_triggered"));
  QVERIFY2(dispatch.elapsed() < 250, "Reload and Preview parsed source in the GUI process");

  auto *progress = window->findChild<ProgressWidget *>();
  QVERIFY(progress != nullptr);
  progress->cancel();
  QTRY_VERIFY_WITH_TIMEOUT(window->computeWorkerProcessId() != worker, 5000);
}

void TestMainWindow::checkF6UsesCommandLineDefinitions()
{
  restoreWindowInitialState();
  const auto previousCommands = commandline_commands;
  commandline_commands = "size = 7;\n";
  window->activeEditor->setPlainText("cube(size);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 10000);
  QCOMPARE(window->rootGeom->getBoundingBox().max().x(), 7.0);
  commandline_commands = previousCommands;
}

#ifdef ENABLE_PYTHON
void TestMainWindow::checkF6UsesTrustedPythonWorker()
{
  restoreWindowInitialState();
  QTemporaryFile file(QDir::tempPath() + "/openscad-python-XXXXXX.py");
  QVERIFY(file.open());
  QVERIFY(file.write("from openscad import cube, show\nshow(cube([7, 1, 1]))\n") > 0);
  QVERIFY(file.flush());
  window->tabManager->open(file.fileName());
  Feature::enable_feature(Feature::ExperimentalPythonEngine.get_name());
  python_trusted = true;

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 10000);
  QCOMPARE(window->rootGeom->getBoundingBox().max().x(), 7.0);
  python_trusted = false;
}
#endif

#include "TestMainWindow.h"

#include <QElapsedTimer>
#include <QProgressBar>
#include <QDoubleSpinBox>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <QSignalSpy>
#include <QVBoxLayout>
#include <QTimer>

#include "gui/OpenSCADApp.h"
#include "gui/QSettingsCached.h"
#include "gui/Console.h"
#include "gui/ComputeWorker.h"
#include "gui/ProgressWidget.h"
#include "gui/parameter/ParameterWidget.h"
#include "glview/Camera.h"
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

void TestMainWindow::checkTestResetDoesNotPersistAutoReload()
{
  QSettingsCached settings;
  const auto before = settings.value("design/autoReload");
  restoreWindowInitialState();
  QCOMPARE(settings.value("design/autoReload"), before);
}

void TestMainWindow::checkOpeningLargeFileDoesNotParseInGui()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
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
  SKIP_WITHOUT_PROCESS_ISOLATION();
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
  SKIP_WITHOUT_PROCESS_ISOLATION();
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
  SKIP_WITHOUT_PROCESS_ISOLATION();
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
  SKIP_WITHOUT_PROCESS_ISOLATION();
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
  const bool isolated = MainWindow::isProcessIsolation();
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
  MainWindow::setProcessIsolation(isolated);
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

// A second F6 on unchanged source must be answered from the worker's geometry cache, and the
// saving must survive the trip back into the GUI. The worker process is persistent precisely so
// that it can be; only the worker side of that was guarded, and a regression is invisible from
// the outside -- the render is still correct, just as slow as the first one.
void TestMainWindow::checkRepeatedF6IsServedFromWorkerCache()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  // Heavy enough that a re-evaluation is unmistakable next to the fixed cost of shipping the
  // result back into the GUI process, which every render pays cached or not.
  window->activeEditor->setPlainText(
    "for (i = [0:60]) rotate([0, 0, i * 6]) translate([10, 0, 0]) sphere(3, $fn = 40);");

  // The macros below return void on failure, so the elapsed time comes back through a reference.
  const auto render = [this](qint64& elapsed) {
    window->rootGeom.reset();
    QElapsedTimer timer;
    timer.start();
    QVERIFY2(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"), "F6 dispatch failed");
    QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 120000);
    elapsed = timer.elapsed();
  };

  // Lazy union is off for the measurement. With it on, every top-level object comes back as its
  // own product and the GUI-side half of a render -- transferring each one and preparing a
  // renderer for it -- costs several seconds either way, which swamps whatever the worker saved
  // and would make this a test of that cost instead of a test of the cache.
  const auto lazyUnion = Feature::ExperimentalLazyUnion.is_enabled();
  Feature::enable_feature(Feature::ExperimentalLazyUnion.get_name(), false);
  qint64 cold = 0, warm = 0;
  render(cold);
  render(warm);
  Feature::enable_feature(Feature::ExperimentalLazyUnion.get_name(), lazyUnion);
  QVERIFY(cold > 0);
  QVERIFY(warm > 0);
  qDebug() << "cold F6:" << cold << "ms, warm F6:" << warm << "ms";
  QVERIFY2(warm < cold / 2,
           qPrintable(QString("repeat F6 on unchanged source took %1ms against a cold %2ms; the "
                              "compute worker's geometry cache is not being hit")
                        .arg(warm)
                        .arg(cold)));
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
  SKIP_WITHOUT_PROCESS_ISOLATION();
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
  SKIP_WITHOUT_PROCESS_ISOLATION();
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
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  // Wait for a live worker rather than sampling one: the preceding test kills this window's
  // worker, and the replacement is started on a short timer, so the PID is briefly 0. Reading
  // it immediately made this test pass or fail on timing alone.
  QTRY_VERIFY_WITH_TIMEOUT(window->computeWorkerProcessId() > 0, 5000);
  const auto worker = window->computeWorkerProcessId();
  window->exitComputeWorkerForTest();
  QTRY_VERIFY_WITH_TIMEOUT(
    window->computeWorkerProcessId() > 0 && window->computeWorkerProcessId() != worker, 5000);
}

void TestMainWindow::checkQueuedRequestsAreNotDroppedBeforeWorkerReady()
{
  restoreWindowInitialState();

  // A stub worker that delays "ready" long enough that both requests below are
  // necessarily queued while ComputeWorker::ready is still false, and that records
  // every command line it actually receives.
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const auto received = directory.filePath("received.txt");
  const auto stub = directory.filePath("stub-worker.sh");
  {
    QFile script(stub);
    QVERIFY(script.open(QIODevice::WriteOnly));
    script.write(QString("#!/bin/sh\n"
                         "sleep 1\n"
                         "echo ready\n"
                         "while IFS= read -r line; do\n"
                         "  [ \"$line\" = quit ] && exit 0\n"
                         "  printf '%s\\n' \"$line\" >> '%1'\n"
                         "done\n")
                   .arg(received)
                   .toUtf8());
  }
  QVERIFY(QFile::setPermissions(stub, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

  ComputeWorker worker(stub);
  const ParameterSet parameters;
  const Camera camera;
  // Both dispatched before returning to the event loop, so neither can have been
  // written to the process yet: this is the worker-startup / respawn window.
  worker.startPreview("cube(1);", {}, parameters, 0, 0.0, camera, false, {});
  worker.startPreview("cube(2);", {}, parameters, 0, 0.0, camera, false, {});

  QStringList commands;
  QTRY_VERIFY_WITH_TIMEOUT(
    [&] {
      QFile file(received);
      if (!file.open(QIODevice::ReadOnly)) return false;
      commands = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
      return commands.size() >= 2;
    }(),
    10000);

  // Both queued requests must reach the worker. Dropping one leaves an orphan
  // RequestContext on the deque, so every later response is matched to the wrong
  // request and the worker never reports itself idle again.
  QCOMPARE(commands.size(), 2);
  QVERIFY(commands[0] != commands[1]);
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
  SKIP_WITHOUT_PROCESS_ISOLATION();
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

void TestMainWindow::checkIdenticalPreviewRequestIsDebounced()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  window->activeEditor->setPlainText("for (i = [0:100000]) translate([i, 0, 0]) cube(1);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  auto *progress = window->findChild<ProgressWidget *>();
  QVERIFY(progress != nullptr);
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QVERIFY(!progress->wasCanceled());
  progress->cancel();
}

void TestMainWindow::checkEditedPreviewRequestReplacesActivePreview()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  window->activeEditor->setPlainText("for (i = [0:100000]) translate([i, 0, 0]) cube(1);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  auto *progress = window->findChild<ProgressWidget *>();
  QVERIFY(progress != nullptr);
  window->activeEditor->setPlainText("cube(2);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QVERIFY(progress->wasCanceled());
  QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 10000);
  QCOMPARE(window->activeEditor->toPlainText(), QString("cube(2);"));
}

void TestMainWindow::checkOpenCSGPreparationCanBeCanceled()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
#ifdef ENABLE_OPENCSG
  restoreWindowInitialState();
  window->activeEditor->setPlainText("for (i = [0:999]) translate([i, 0, 0]) cube(1);");

  QTimer cancelWhenPreparing;
  cancelWhenPreparing.setInterval(1);
  connect(&cancelWhenPreparing, &QTimer::timeout, window, [this, &cancelWhenPreparing]() {
    // MainWindow::compileCSG() only assigns previewRenderer *after* prepare()
    // returns, so waiting for it here waits for a window that never opens. A
    // non-zero GUI progress value is raised from inside the prepare callback,
    // which is exactly the moment this test wants to cancel in.
    auto *progress = window->findChild<ProgressWidget *>();
    if (progress && progress->guiValue() > 0) {
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

void TestMainWindow::checkWorkerCompletionDoesNotFinishPreviewProgress()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
#ifdef ENABLE_OPENCSG
  restoreWindowInitialState();
  window->activeEditor->setPlainText("for (i = [0:999]) translate([i, 0, 0]) cube(1);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  auto *progress = window->findChild<ProgressWidget *>();
  QVERIFY(progress != nullptr);
  QTRY_VERIFY_WITH_TIMEOUT(progress->value() > 0, 10000);
  QVERIFY(progress->value() < 1000);
  progress->cancel();
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
#endif
}

void TestMainWindow::checkPreviewShowsSeparateGuiProgress()
{
  ProgressWidget render;
  const auto bars = render.findChildren<QProgressBar *>();
  QCOMPARE(bars.size(), 2);
  QVERIFY(render.progressBar->styleSheet().isEmpty());
  QVERIFY(render.guiProgressBar->styleSheet().isEmpty());
  // A render (F6) has a single phase, so it shows one full-width bar and never a permanently
  // empty second one. The shape of the panel is itself the indicator of which kind of
  // operation is running.
  QVERIFY(!render.progressBar->isHidden());
  QVERIFY(render.guiProgressBar->isHidden());

  // A preview (F5) has two phases -- the worker's, then the GUI's -- and must show both bars
  // from the start. Revealing the second one only once the worker phase finished made the first
  // bar shrink mid-operation, which reads as the layout glitching rather than as a second phase
  // beginning.
  // isVisible() is false for any child of a top-level widget that was never shown, which this
  // one is not; isHidden() is what "the bar is showing" actually means here.
  ProgressWidget preview(nullptr, true);
  QVERIFY(!preview.progressBar->isHidden());
  QVERIFY(!preview.guiProgressBar->isHidden());
  QCOMPARE(preview.guiValue(), 0);

  // startGuiProgress supplies the real maximum once the worker's products are in. It is no
  // longer what makes the bar appear, and it must not disturb one that is already showing.
  preview.startGuiProgress(10);
  QVERIFY(!preview.guiProgressBar->isHidden());
  QCOMPARE(preview.guiValue(), 0);
  preview.setGuiValue(5);
  QCOMPARE(preview.guiValue(), 5);
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
  window->qglview->setColorScheme("Starnight");
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

// The legacy preview path ends in compileCSG(), which logs "Compile and preview finished." and the
// total rendering time. The isolated path ends in actionPreviewDone() instead, and was logging
// neither, so an isolated F5 left the console with no indication of how long it took.
void TestMainWindow::checkPreviewReportsRenderingTime()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  window->previewRenderer.reset();
  window->console->clear();
  window->activeEditor->setPlainText("#cube(1);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
#ifdef ENABLE_OPENCSG
  QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 10000);
#else
  QTRY_VERIFY_WITH_TIMEOUT(window->thrownTogetherRenderer != nullptr, 10000);
#endif
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 10000);
  // The console is fed through queued signals, so give it a moment to catch up.
  QTRY_VERIFY_WITH_TIMEOUT(window->console->toPlainText().contains("Compile and preview finished."),
                           5000);
  QVERIFY2(window->console->toPlainText().contains("Total rendering time:"),
           "isolated preview did not report its rendering time");
}

// One F5 after an edit must show the edited model. The user reports seeing the previous geometry
// until F5 is pressed a second time, which would mean the preview displayed is one request stale.
void TestMainWindow::checkEditedSourcePreviewsOnTheFirstF5()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();

  const auto previewAndMeasure = [this](const QString& source, double& width) {
    window->previewRenderer.reset();
    window->activeEditor->setPlainText(source);
    QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
    QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 10000);
    QVERIFY(window->previewProductsForTest() != nullptr);
    width = window->previewProductsForTest()->getBoundingBox().max().x();
  };

  double first = 0, second = 0;
  previewAndMeasure("cube(10);", first);
  QCOMPARE(first, 10.0);
  previewAndMeasure("cube(30);", second);
  QCOMPARE(second, 30.0);
}

// F5 pressed while a previous preview is still running. The second request must be the one that
// ends up on screen; if the in-flight one wins, the user sees the model they just edited away from
// and has to press F5 again -- which is the reported symptom.
void TestMainWindow::checkF5DuringAnInFlightPreviewShowsTheEditedModel()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  window->previewRenderer.reset();

  // Slow enough that the second F5 lands while it is still being computed.
  window->activeEditor->setPlainText("for (i = [0:400]) translate([i, 0, 0]) sphere(1, $fn = 40);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));

  window->activeEditor->setPlainText("cube(30);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));

  QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 60000);
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 60000);
  QVERIFY(window->previewProductsForTest() != nullptr);
  QCOMPARE(window->previewProductsForTest()->getBoundingBox().max().x(), 30.0);
}

// Edit, preview, edit, preview -- many times, checking what is actually ON SCREEN each round.
// The user reports the view intermittently keeping the previous model until F5 is pressed again.
// Asserting on rootProduct cannot see that: the products can be correct while the view still
// draws the old ones. This grabs the GL framebuffer and counts drawn pixels instead.
void TestMainWindow::checkRepeatedEditPreviewCyclesDrawTheEditedModel()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  window->show();
  QTest::qWaitForWindowExposed(window);

  const auto drawnPixels = [this] {
    window->qglview->update();
    QApplication::processEvents();
    const auto image = window->qglview->grabFramebuffer();
    const auto background = image.pixel(0, 0);
    qint64 drawn = 0;
    for (int y = 0; y < image.height(); ++y) {
      for (int x = 0; x < image.width(); ++x) {
        if (image.pixel(x, y) != background) ++drawn;
      }
    }
    return drawn;
  };

  // Two models whose silhouettes differ a lot, so "which one is on screen" is unambiguous.
  const QString small = "cube(6, center = true);";
  const QString large = "cube(60, center = true);";
  const auto preview = [this](const QString& source) {
    window->previewRenderer.reset();
    window->activeEditor->setPlainText(source);
    QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
    QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 20000);
    QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 20000);
  };

  // The real app polls for auto-reload every 200ms while the user works; restoreWindowInitialState()
  // ticks the menu item with signals blocked, so the timer is NOT running in the other tests. The
  // reported symptom appears during ordinary edit/preview loops, where it IS running -- a poll can
  // land between an edit and its preview, so the test has to include that traffic.
  window->on_designActionAutoReload_toggled(true);

  preview(large);
  const auto largePixels = drawnPixels();
  preview(small);
  const auto smallPixels = drawnPixels();
  if (largePixels == 0 || largePixels <= smallPixels) {
    QSKIP("this display cannot distinguish the two models -- no usable framebuffer to assert on");
  }
  const auto threshold = (largePixels + smallPixels) / 2;

  // The reported failure is intermittent, so one round proves nothing.
  for (int cycle = 0; cycle < 12; ++cycle) {
    const bool wantLarge = cycle % 2 == 0;
    preview(wantLarge ? large : small);
    const auto drawn = drawnPixels();
    const bool showsLarge = drawn > threshold;
    QVERIFY2(showsLarge == wantLarge,
             qPrintable(QString("cycle %1: asked for the %2 cube, screen shows the %3 one "
                                "(%4 pixels drawn, large=%5 small=%6)")
                          .arg(cycle)
                          .arg(wantLarge ? "large" : "small")
                          .arg(showsLarge ? "large" : "small")
                          .arg(drawn)
                          .arg(largePixels)
                          .arg(smallPixels)));
  }

  // A person does not wait for quiescence before typing the next edit. Interrupt each preview at a
  // different point, so the second request lands during dispatch, during computation, and during
  // the GUI-side OpenCSG preparation across the run.
  for (int cycle = 0; cycle < 12; ++cycle) {
    window->activeEditor->setPlainText(cycle % 2 == 0 ? small : large);
    QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
    QTest::qWait(5 + cycle * 13);

    const bool wantLarge = cycle % 2 == 0;
    window->previewRenderer.reset();
    window->activeEditor->setPlainText(wantLarge ? large : small);
    QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
    QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 20000);
    QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 20000);
    // Nothing else is in flight now, so the screen must agree with the last edit.
    QTRY_VERIFY_WITH_TIMEOUT(!window->computeBusyForTest(), 20000);

    const auto drawn = drawnPixels();
    const bool showsLarge = drawn > threshold;
    QVERIFY2(showsLarge == wantLarge,
             qPrintable(QString("interrupted cycle %1 (waited %2ms): asked for the %3 cube, screen "
                                "shows the %4 one (%5 pixels, large=%6 small=%7)")
                          .arg(cycle)
                          .arg(5 + cycle * 13)
                          .arg(wantLarge ? "large" : "small")
                          .arg(showsLarge ? "large" : "small")
                          .arg(drawn)
                          .arg(largePixels)
                          .arg(smallPixels)));
  }
}

void TestMainWindow::checkCustomizerIsUsableAfterAnIsolatedRender()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  window->activeEditor->parameterWidget->setEnabled(false);

  window->activeEditor->setPlainText("size = 10; // [10:100]\ncube(size);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 20000);
  QTRY_VERIFY_WITH_TIMEOUT(window->activeEditor->parameterWidget->isEnabled(), 5000);
}

void TestMainWindow::checkRightClickAfterIsolatedPreviewDoesNotCrash()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  window->activeEditor->setPlainText("cube(1);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 10000);
  QVERIFY(!window->previewSelectionPath(1).empty());
}

void TestMainWindow::checkReloadPreviewDispatchDoesNotBlockGui()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
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

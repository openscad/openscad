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
#include <QThread>
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

// Two windows previewing at once must not take twice as long as one. The OpenCSG
// preparation phase (VBOBuilder::create_surface over every CSG leaf) is CPU-bound,
// GL-free and per-window, so it belongs off the main thread; while it ran on it, the
// second window's preparation could only start after the first one's had finished, and
// this measured a ~2x wall-clock cost for two windows.
//
// The model is deliberately mesh-heavy per leaf (36 leaves at $fn=64): that puts ~87%
// of the preparation phase in create_surface, so the ratio measures the phase this test
// is about rather than per-leaf progress overhead. Timing-based, so the threshold is
// loose -- 1.5x sits between the ~1.7x this fails at when preparation is serialized and
// the ~1.1x it passes at when it is not.
void TestMainWindow::checkWindowsPrepareOpenCSGConcurrently()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  if (QThread::idealThreadCount() < 2) QSKIP("needs more than one core");
  restoreWindowInitialState();

  const auto model = [](int row) {
    return QString(
             "for (i = [0:17]) translate([i * 12, %1, 0]) difference() { sphere(6, $fn = 64); "
             "cylinder(h = 20, r = 3, center = true, $fn = 64); }")
      .arg(row * 12);
  };
  const auto previewAndWait = [](std::initializer_list<MainWindow *> windows) {
    QElapsedTimer timer;
    timer.start();
    for (auto *w : windows) {
      if (!QMetaObject::invokeMethod(w, "on_designActionPreview_triggered")) return qint64{-1};
    }
    for (auto *w : windows) {
      if (!QTest::qWaitFor([w]() { return w->findChild<ProgressWidget *>() == nullptr; }, 120000)) {
        return qint64{-1};
      }
    }
    return timer.elapsed();
  };

  window->activeEditor->setPlainText(model(0));
  const qint64 single = previewAndWait({window});
  QVERIFY2(single > 200, "baseline preview was too fast to measure -- model too small?");

  auto *other = new MainWindow({});
  window->activeEditor->setPlainText(model(1));
  other->activeEditor->setPlainText(model(2));
  const qint64 concurrent = previewAndWait({window, other});
  other->close();

  QVERIFY2(concurrent > 0 && concurrent < single * 3 / 2,
           qPrintable(QString("two windows took %1 ms against %2 ms for one (%3x); the GUI-side "
                              "preparation phase is still serializing on the main thread")
                        .arg(concurrent)
                        .arg(single)
                        .arg(static_cast<double>(concurrent) / single, 0, 'f', 2)));
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

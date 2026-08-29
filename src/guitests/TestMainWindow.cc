#include "TestMainWindow.h"

#include <QCheckBox>
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
#include "gui/Preferences.h"
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

  window->tabManager->open(file.fileName());

  bool eventDelivered = false;
  QTimer::singleShot(0, window, [&eventDelivered]() { eventDelivered = true; });
  QTRY_VERIFY(eventDelivered);

  if (auto *progress = window->findChild<ProgressWidget *>()) {
    progress->cancel();
    QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
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

// Hold the CPU-bound half of each window's preparation at a test barrier and observe that both
// enter it. This proves overlap directly without confusing parallelism with a wall-clock ratio.
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
  auto *other = new MainWindow({});
  window->activeEditor->setPlainText(model(1));
  other->activeEditor->setPlainText(model(2));
  MainWindow::holdOpenCSGPreparationsForTest();
  const bool firstStarted = QMetaObject::invokeMethod(window, "on_designActionPreview_triggered");
  const bool secondStarted = QMetaObject::invokeMethod(other, "on_designActionPreview_triggered");
  const bool overlapped =
    QTest::qWaitFor([]() { return MainWindow::heldOpenCSGPreparationsForTest() == 2; }, 10000);
  MainWindow::releaseOpenCSGPreparationsForTest();

  QVERIFY(firstStarted);
  QVERIFY(secondStarted);
  QVERIFY2(overlapped, "both windows did not enter CPU-side OpenCSG preparation concurrently");
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 120000);
  QTRY_VERIFY_WITH_TIMEOUT(other->findChild<ProgressWidget *>() == nullptr, 120000);
  other->close();
}

// The off-GUI-thread preparation is what lets two isolated windows work at once, but it is a
// threading change in the common preview path, so it is gated on process isolation. With the
// feature off the preparation must run inline on the GUI thread, which means the worker-thread
// hold below is never reached and the preview finishes regardless of it.
// Streaming preview and structured diagnostics do nothing without process isolation, so the
// Features page has to say so: they are indented under it and greyed out while it is off.
void TestMainWindow::checkDependentFeatureCheckBoxesFollowProcessIsolation()
{
  auto *prefs = GlobalPreferences::inst();
  QVERIFY(prefs != nullptr);

  const auto boxFor = [prefs](const char *name) -> QCheckBox * {
    for (auto *cb : prefs->findChildren<QCheckBox *>()) {
      if (cb->text() == QString(name)) return cb;
    }
    return nullptr;
  };
  auto *isolation = boxFor("process-isolation");
  auto *streaming = boxFor("streaming-preview");
  auto *diagnostics = boxFor("structured-diagnostics");
  QVERIFY(isolation != nullptr);
  QVERIFY(streaming != nullptr);
  QVERIFY(diagnostics != nullptr);

  const bool wasChecked = isolation->isChecked();

  isolation->setChecked(false);
  QVERIFY2(!streaming->isEnabled(), "streaming-preview stayed enabled without process isolation");
  QVERIFY2(!diagnostics->isEnabled(), "structured-diagnostics stayed enabled without process isolation");

  isolation->setChecked(true);
  QVERIFY(streaming->isEnabled());
  QVERIFY(diagnostics->isEnabled());

  isolation->setChecked(wasChecked);
}

void TestMainWindow::checkLegacyPreviewPreparesOnGuiThread()
{
  SKIP_WITH_PROCESS_ISOLATION();
  restoreWindowInitialState();

  window->activeEditor->setPlainText(
    "difference() { sphere(6, $fn = 64); cylinder(h = 20, r = 3, center = true, $fn = 64); }");
  window->previewRenderer.reset();
  MainWindow::holdOpenCSGPreparationsForTest();
  const bool started = QMetaObject::invokeMethod(window, "on_designActionPreview_triggered");
  // previewRenderer is only assigned once preparation has finished, so this cannot pass by
  // running before preparation starts the way a progress-widget check can.
  const bool finished = QTest::qWaitFor([this]() { return window->previewRenderer != nullptr; }, 60000);
  const int held = MainWindow::heldOpenCSGPreparationsForTest();
  MainWindow::releaseOpenCSGPreparationsForTest();

  QVERIFY(started);
  QCOMPARE(held, 0);
  QVERIFY2(finished, "the legacy preview did not finish; preparation went to a worker thread");
}

void TestMainWindow::checkWorkerMessageSeverity()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  restoreWindowInitialState();
  window->console->clear();
  auto *collapse = window->console->findChild<QAction *>("actionCollapseDiagnostics");
  QVERIFY(collapse != nullptr);
  collapse->setChecked(true);
  Feature::enable_feature(Feature::ExperimentalStructuredDiagnostics.get_name(), false);
  window->tabManager->open(QString::fromStdString(PlatformUtils::resourceBasePath()) +
                           "/tests/basic-ux/empty.scad");
  window->activeEditor->setPlainText("for (i = [0:7]) echo(missing);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QTRY_VERIFY_WITH_TIMEOUT(window->console->toPlainText().contains("Ignoring unknown variable"), 5000);
  QVERIFY(!window->console->toPlainText().contains("occurred 8 times"));
  QVERIFY(window->console->unabridgedText().isEmpty());

  Feature::enable_feature(Feature::ExperimentalStructuredDiagnostics.get_name());
  window->console->clear();
  window->activeEditor->setPlainText(
    "echo(\"ordinary\");\n"
    "for (i = [0:7]) echo(missing);\n"
    "cube(1);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QElapsedTimer outputTimer;
  outputTimer.start();
  while (!window->console->toPlainText().contains("ordinary") && outputTimer.elapsed() < 5000) {
    QTest::qWait(50);
  }
  QVERIFY2(window->console->toPlainText().contains("ordinary"),
           qPrintable(QString("console output: %1").arg(window->console->toPlainText())));
  QVERIFY(window->console->toPlainText().contains("occurred 8 times"));
  QCOMPARE(window->console->unabridgedText().count("Ignoring unknown variable"), 8);
  QCOMPARE(window->compilationWarningCount(), 8);

  collapse->setChecked(false);
  window->console->clear();
  window->activeEditor->setPlainText("for (i = [0:2]) echo(missing);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QTRY_COMPARE_WITH_TIMEOUT(window->console->toPlainText().count("Ignoring unknown variable"), 3, 5000);
  QVERIFY(!window->console->toPlainText().contains("occurred"));
  collapse->setChecked(true);

  window->console->clear();
  window->activeEditor->setPlainText("for (i = [1:3]) rotate([i, i, i, i]) cube(1);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QTRY_VERIFY_WITH_TIMEOUT(window->console->toPlainText().contains("occurred 3 times"), 5000);
  QCOMPARE(window->console->toPlainText().count("Problem converting rotate"), 2);
  QCOMPARE(window->console->unabridgedText().count("Problem converting rotate"), 3);
  QCOMPARE(window->compilationWarningCount(), 3);

  window->activeEditor->setPlainText("assert(false, \"failure\");");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
  QTRY_VERIFY_WITH_TIMEOUT(window->console->toPlainText().contains("failure"), 5000);
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
  QElapsedTimer timer;
  timer.start();
  while (window->rootGeom == nullptr && timer.elapsed() < 10000) QTest::qWait(50);
  QVERIFY2(window->rootGeom != nullptr, qPrintable(window->console->toPlainText()));
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
  window->activeEditor->setPlainText(
    "for (i = [0:60]) rotate([0, 0, i * 6]) translate([10, 0, 0]) sphere(3, $fn = 40);");

  const auto worker = window->computeWorkerProcessId();
  const auto render = [this](BoundingBox& bounds) {
    window->rootGeom.reset();
    QVERIFY2(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"), "F6 dispatch failed");
    QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 120000);
    bounds = window->rootGeom->getBoundingBox();
  };

  // Lazy union is off for the measurement. With it on, every top-level object comes back as its
  // own product and the GUI-side half of a render -- transferring each one and preparing a
  // renderer for it -- costs several seconds either way, which swamps whatever the worker saved
  // and would make this a test of that cost instead of a test of the cache.
  const auto lazyUnion = Feature::ExperimentalLazyUnion.is_enabled();
  Feature::enable_feature(Feature::ExperimentalLazyUnion.get_name(), false);
  BoundingBox firstBounds, secondBounds;
  render(firstBounds);
  QCOMPARE(window->computeWorkerProcessId(), worker);
  render(secondBounds);
  Feature::enable_feature(Feature::ExperimentalLazyUnion.get_name(), lazyUnion);
  QCOMPARE(window->computeWorkerProcessId(), worker);
  QCOMPARE(secondBounds.min(), firstBounds.min());
  QCOMPARE(secondBounds.max(), firstBounds.max());
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
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const auto started = directory.filePath("started");
  const auto stub = directory.filePath("unresponsive-worker.sh");
  QFile script(stub);
  QVERIFY(script.open(QIODevice::WriteOnly));
  script.write(QString("#!/bin/sh\n"
                       "echo ready\n"
                       "IFS= read -r request\n"
                       "touch '%1'\n"
                       "IFS= read -r ignored\n")
                 .arg(started)
                 .toUtf8());
  script.close();
  QVERIFY(QFile::setPermissions(stub, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

  ComputeWorker worker(stub);
  QTRY_VERIFY_WITH_TIMEOUT(worker.processId() > 0, 5000);
  const auto originalPid = worker.processId();
  worker.start("cube(1);", {}, {}, 0.0, {}, false, {});
  QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(started), 5000);
  worker.cancel();
  QTRY_VERIFY_WITH_TIMEOUT(worker.processId() > 0 && worker.processId() != originalPid, 5000);
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
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));

  auto *progress = window->findChild<ProgressWidget *>();
  QVERIFY(progress != nullptr);
  bool eventDelivered = false;
  QTimer::singleShot(0, window, [&eventDelivered]() { eventDelivered = true; });
  QTRY_VERIFY(eventDelivered);
  progress->cancel();
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
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

  MainWindow::holdOpenCSGPreparationsForTest();
  const bool started = QMetaObject::invokeMethod(window, "on_designActionPreview_triggered");
  const bool enteredPreparation =
    QTest::qWaitFor([]() { return MainWindow::heldOpenCSGPreparationsForTest() == 1; }, 10000);
  auto *progress = window->findChild<ProgressWidget *>();
  if (progress) progress->cancel();
  MainWindow::releaseOpenCSGPreparationsForTest();

  QVERIFY(started);
  QVERIFY2(enteredPreparation, "preview never entered CPU-side OpenCSG preparation");
  QVERIFY(progress != nullptr);
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

// Benchmark for feature 34, skipped unless OPENSCAD_STREAMING_BENCH names a .scad file;
// OPENSCAD_STREAMING_BENCH_FLAG picks the flag state. Kept because this number has to be
// re-measured after any change to the GUI-side preview phase, and rebuilding the harness each
// time costs more than carrying it.
//
// One preview, one process, one flag state -- deliberately. An earlier cut ran both states in a
// single process and reported streaming 9.8% faster, which it could not distinguish from the
// second preview simply reusing the first one's caches. Separate processes remove that entirely.
// The two processes are then launched concurrently so both meet the same machine load; running
// them one after another lets anything that starts in between masquerade as an effect.
void TestMainWindow::checkStreamingPreviewBenchmark()
{
  const auto model = qEnvironmentVariable("OPENSCAD_STREAMING_BENCH");
  if (model.isEmpty()) QSKIP("set OPENSCAD_STREAMING_BENCH to a .scad path to run this");
  SKIP_WITHOUT_PROCESS_ISOLATION();
  const auto streaming = qEnvironmentVariable("OPENSCAD_STREAMING_BENCH_FLAG") == "1";

  Feature::enable_feature(Feature::ExperimentalStreamingPreview.get_name(), streaming);
  restoreWindowInitialState();
  window->tabManager->open(model);
  QElapsedTimer timer;
  timer.start();
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 3600000);
  const auto ms = timer.elapsed();

  qDebug("STREAMING BENCH streaming=%d %lld ms %zu products", streaming ? 1 : 0, ms,
         window->previewProductCount());
}

void TestMainWindow::checkStreamingPreviewProducesSameResult()
{
  SKIP_WITHOUT_PROCESS_ISOLATION();
  // The flag changes when leaf geometry is decoded, never what it decodes to. A preview with it
  // on must produce the same renderer state as one with it off; if it does not, the streamed
  // decode path and the read-at-the-end path have diverged.
  //
  // It also covers the no-restart claim: the flag is read when a preview is dispatched, so
  // toggling it between two previews in one window has to take effect without restarting.
  const auto wasEnabled = Feature::ExperimentalStreamingPreview.is_enabled();
  // Void, because QVERIFY expands to a bare `return;` and cannot live in a lambda that returns
  // a value. The count comes back through the out-parameter instead.
  const auto previewOnce = [this](size_t& products) {
    restoreWindowInitialState();
    window->activeEditor->setPlainText("#cube(1); translate([2, 0, 0]) sphere(1, $fn = 12);");
    QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
#ifdef ENABLE_OPENCSG
    QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 10000);
#else
    QTRY_VERIFY_WITH_TIMEOUT(window->thrownTogetherRenderer != nullptr, 10000);
#endif
    QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 10000);
    products = window->previewProductCount();
  };

  size_t withoutStreaming = 0;
  size_t withStreaming = 0;
  Feature::enable_feature(Feature::ExperimentalStreamingPreview.get_name(), false);
  previewOnce(withoutStreaming);
  Feature::enable_feature(Feature::ExperimentalStreamingPreview.get_name());
  previewOnce(withStreaming);
  Feature::enable_feature(Feature::ExperimentalStreamingPreview.get_name(), wasEnabled);

  QVERIFY(withoutStreaming > 0);
  QCOMPARE(withStreaming, withoutStreaming);
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
    window->qglview->repaint();
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
  qint64 smallPixels = largePixels;
  QTRY_VERIFY_WITH_TIMEOUT((smallPixels = drawnPixels()) < largePixels, 5000);
  if (largePixels == 0 || largePixels <= smallPixels) {
    QSKIP("this display cannot distinguish the two models -- no usable framebuffer to assert on");
  }
  const auto threshold = (largePixels + smallPixels) / 2;

  // The reported failure is intermittent, so one round proves nothing.
  for (int cycle = 0; cycle < 12; ++cycle) {
    const bool wantLarge = cycle % 2 == 0;
    preview(wantLarge ? large : small);
    qint64 drawn = 0;
    const bool painted = QTest::qWaitFor(
      [&]() {
        drawn = drawnPixels();
        return (drawn > threshold) == wantLarge;
      },
      5000);
    QVERIFY2(painted, qPrintable(QString("cycle %1: asked for the %2 cube, screen shows the %3 one "
                                         "(%4 pixels drawn, large=%5 small=%6)")
                                   .arg(cycle)
                                   .arg(wantLarge ? "large" : "small")
                                   .arg(drawn > threshold ? "large" : "small")
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
    QTRY_VERIFY_WITH_TIMEOUT(window->computeBusyForTest(), 5000);

    const bool wantLarge = cycle % 2 == 0;
    window->previewRenderer.reset();
    window->activeEditor->setPlainText(wantLarge ? large : small);
    QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
    QTRY_VERIFY_WITH_TIMEOUT(window->previewRenderer != nullptr, 20000);
    QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 20000);
    // Nothing else is in flight now, so the screen must agree with the last edit.
    QTRY_VERIFY_WITH_TIMEOUT(!window->computeBusyForTest(), 20000);

    qint64 drawn = 0;
    const bool painted = QTest::qWaitFor(
      [&]() {
        drawn = drawnPixels();
        return (drawn > threshold) == wantLarge;
      },
      5000);
    QVERIFY2(painted,
             qPrintable(QString("interrupted cycle %1: asked for the %2 cube, screen shows the %3 "
                                "one (%4 pixels, large=%5 small=%6)")
                          .arg(cycle)
                          .arg(wantLarge ? "large" : "small")
                          .arg(drawn > threshold ? "large" : "small")
                          .arg(drawn)
                          .arg(largePixels)
                          .arg(smallPixels)));
  }
}

// The Customizer's policy is that once the user touches a value it wins until the document closes.
// A value the user has NEVER touched must not win: editing the variable's default in the text has
// to take effect on the first render, as it does in legacy mode. Isolated mode used to send the
// widget's values unconditionally, so every edit rendered one step behind -- and F6 then exported
// that stale mesh. Both halves are asserted here, and no isolation skip: legacy must agree.
void TestMainWindow::checkUntouchedCustomizerDoesNotOverrideEditedText()
{
  restoreWindowInitialState();

  // A Customizer value outlives a complete source replacement, so this uses a name no other test
  // touches -- "size" would leak 70 into checkF6UsesCommandLineDefinitions, which sets size via -D.
  window->activeEditor->setPlainText("outrank_size = 10; // [10:100]\ncube(outrank_size);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 20000);
  QCOMPARE(window->rootGeom->getBoundingBox().max().x(), 10.0);

  // An untouched Customizer must not override the edit: legacy renders 40 here, and isolated mode
  // rendered 10 until this was fixed.
  window->rootGeom.reset();
  window->activeEditor->setPlainText("outrank_size = 40; // [10:100]\ncube(outrank_size);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 20000);
  QCOMPARE(window->rootGeom->getBoundingBox().max().x(), 40.0);

  // ...and the Customizer does, which is the escape hatch the user needs to be able to reach.
  // Look the widget up now, not earlier: setParameters() rebuilds them whenever the source
  // changes, so a pointer taken before the render above is dangling by this point.
  window->rootGeom.reset();
  auto *spinBox = window->activeEditor->parameterWidget->findChild<QDoubleSpinBox *>("doubleSpinBox");
  QVERIFY(spinBox != nullptr);
  spinBox->setValue(70);
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 20000);
  QCOMPARE(window->rootGeom->getBoundingBox().max().x(), 70.0);
}
// A plain top-level variable with no customizer annotation at all. Every top-level assignment is a
// Customizer parameter in OpenSCAD whether or not it carries a comment, so if the widget's values
// are exported unconditionally this goes stale exactly like an annotated one -- with the Customizer
// pane closed and never touched, which is how the user hit it.
void TestMainWindow::checkEditingAPlainTopLevelVariableTakesEffect()
{
  // (no isolation skip: the point is to compare isolated and legacy)
  restoreWindowInitialState();

  // A name no other test has used: a Customizer value set under one name outlives a complete
  // source replacement, so reusing "size" here would inherit the previous test's 70.
  window->activeEditor->setPlainText("plain_width = 10;\ncube(plain_width);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 20000);
  QCOMPARE(window->rootGeom->getBoundingBox().max().x(), 10.0);

  window->rootGeom.reset();
  window->activeEditor->setPlainText("plain_width = 40;\ncube(plain_width);");
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->rootGeom != nullptr, 20000);
  QCOMPARE(window->rootGeom->getBoundingBox().max().x(), 40.0);
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
  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionReloadAndPreview_triggered"));

  auto *progress = window->findChild<ProgressWidget *>();
  QVERIFY(progress != nullptr);
  bool eventDelivered = false;
  QTimer::singleShot(0, window, [&eventDelivered]() { eventDelivered = true; });
  QTRY_VERIFY(eventDelivered);
  progress->cancel();
  QTRY_VERIFY_WITH_TIMEOUT(window->findChild<ProgressWidget *>() == nullptr, 5000);
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

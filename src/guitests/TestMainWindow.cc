#include "TestMainWindow.h"

#include <QScopeGuard>

#include <QString>
#include <QStringList>
#include <QTest>

#include "core/CSGNode.h"
#include "geometry/PolySet.h"
#include "glview/preview/OpenCSGRenderer.h"
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

void TestMainWindow::checkRepeatPreviewOfManyProductsReusesCachedBuffers()
{
#ifdef ENABLE_OPENCSG
  // 150 separate cubes are 150 products. A cache capped at 100 entries evicts part of the model
  // it just built, so previewing it again rebuilds those products every time -- exactly the
  // heavy models the cache exists to help.
  restoreWindowInitialState();
  window->show();
  QVERIFY(QTest::qWaitForWindowExposed(window));
  window->activeEditor->setPlainText("for (i = [0:149]) translate([i * 3, 0, 0]) cube(1);");

  // Wait on the compile itself: a new renderer can be allocated at the old one's address, so
  // watching the pointer change is not a reliable signal.
  const auto previewAndPaint = [this]() {
    bool compiled = false;
    const auto connection = QObject::connect(window, &MainWindow::compilationDone, window,
                                             [&compiled](SourceFile *) { compiled = true; });
    QMetaObject::invokeMethod(window, "on_designActionPreview_triggered");
    QTRY_VERIFY_WITH_TIMEOUT(compiled, 20000);
    QObject::disconnect(connection);
    window->qglview->repaint();
  };

  const auto rootProducts = [this]() -> std::shared_ptr<CSGProducts> {
    const auto renderer = std::dynamic_pointer_cast<OpenCSGRenderer>(window->previewRenderer);
    return renderer ? renderer->rootProductsForTest() : nullptr;
  };
  // Held, not just its address: once the first renderer is gone its leaf could be freed and a new
  // one allocated at the same address, which would look like reuse.
  const auto firstLeaf = [&rootProducts]() -> std::shared_ptr<const PolySet> {
    const auto products = rootProducts();
    if (!products || products->products.empty()) return nullptr;
    const auto& product = products->products.front();
    return product.intersections.empty() ? nullptr : product.intersections.front().leaf->polyset;
  };

  const auto beforeFirst = OpenCSGRenderer::vboBuildsForTest();
  previewAndPaint();
  const auto afterFirst = OpenCSGRenderer::vboBuildsForTest();
  // The first paint has to have built the model, or the repeat's count measures the first build.
  QVERIFY2(afterFirst - beforeFirst >= 150,
           qPrintable(QStringLiteral("the first preview's paint built %1 products, expected 150")
                        .arg(static_cast<qulonglong>(afterFirst - beforeFirst))));
  const auto leafBefore = firstLeaf();
  const auto productCount = rootProducts() ? rootProducts()->products.size() : 0;
  previewAndPaint();
  const auto rebuilt = OpenCSGRenderer::vboBuildsForTest() - afterFirst;
  // The cache is keyed by PolySet identity, so this has to hold before the count means anything.
  QVERIFY2(leafBefore && leafBefore == firstLeaf(),
           qPrintable(QStringLiteral("repeat preview gave new leaf geometry (%1 products)")
                        .arg(static_cast<qulonglong>(productCount))));
  QVERIFY2(rebuilt == 0, qPrintable(QStringLiteral("the repeat preview rebuilt %1 of 150 products")
                                      .arg(static_cast<qulonglong>(rebuilt))));
#endif
}

// The thrown-together view keeps its own vertex buffers, built the same way and cached the same
// way, so it goes stale on a scheme change for the same reason the OpenCSG preview did.
void TestMainWindow::checkChangingColorSchemeRecolorsThrownTogether()
{
  restoreWindowInitialState();
  window->show();
  QVERIFY(QTest::qWaitForWindowExposed(window));
  // This test changes the view mode and the color scheme, which are shared with every test that
  // runs after it -- and with the user's own settings. Put both back on every exit path.
  const bool thrownTogetherWas = window->viewActionThrownTogether->isChecked();
  const bool previewWas = window->viewActionPreview->isChecked();
  auto restore = qScopeGuard([this, thrownTogetherWas, previewWas] {
    window->viewActionThrownTogether->setChecked(thrownTogetherWas);
    window->viewActionPreview->setChecked(previewWas);
    if (previewWas) QMetaObject::invokeMethod(window, "viewModePreview");
    window->qglview->setColorScheme("Cornfield");
  });
  window->qglview->setColorScheme("Cornfield");
  window->activeEditor->setPlainText("cube(100, center = true);");
  window->viewActionThrownTogether->setChecked(true);

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionPreview_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->thrownTogetherRenderer != nullptr, 10000);
  QVERIFY(QMetaObject::invokeMethod(window, "viewModeThrownTogether"));
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
           "Changing schemes left the thrown-together view colored by the previous scheme");
}

// The rendered (F6) view caches its buffers in PolySetRenderer / CGALRenderer.
void TestMainWindow::checkChangingColorSchemeRecolorsRender()
{
  restoreWindowInitialState();
  window->show();
  QVERIFY(QTest::qWaitForWindowExposed(window));
  auto restoreScheme = qScopeGuard([this] { window->qglview->setColorScheme("Cornfield"); });
  window->qglview->setColorScheme("Cornfield");
  window->activeEditor->setPlainText("cube(100, center = true);");

  QVERIFY(QMetaObject::invokeMethod(window, "on_designActionRender_triggered"));
  QTRY_VERIFY_WITH_TIMEOUT(window->qglview->getRenderer() != nullptr, 60000);
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
           "Changing schemes left the rendered view colored by the previous scheme");
}

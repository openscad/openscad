#include "TestExportImage.h"

#include <algorithm>
#include <cmath>
#include <QImage>
#include <QApplication>
#include <QDialog>
#include <QTest>

#include "gui/QGLView.h"
#include "Feature.h"

// The GUI image export does not go through write_png() -- it grabs the widget's framebuffer as a
// QImage and saves that -- so the alpha handling has to be verified separately from the CLI path.
// These tests use the background alone: whatever model is loaded, the corner of the viewport is
// background, which is the pixel transparency is supposed to change.

void TestExportImage::checkGrabFrameIsOpaqueByDefault()
{
  restoreWindowInitialState();

  const QImage frame = window->qglview->grabFrame(false);

  QVERIFY(!frame.isNull());
  // Default export must stay fully opaque, whatever the widget's own surface format is.
  QCOMPARE(qAlpha(frame.pixel(0, 0)), 255);
}

void TestExportImage::checkTransparentGrabHasTransparentBackground()
{
  restoreWindowInitialState();

  const QImage frame = window->qglview->grabFrame(true);

  QVERIFY(!frame.isNull());
  // Deliberately no hasAlphaChannel() check: grabFrame() converts to an alpha-capable format, so
  // that would pass even when the surface format carries no alpha and every pixel comes back
  // opaque. The alpha *values* are the only assertion with teeth here.
  // The corners are background in any camera position.
  QCOMPARE(qAlpha(frame.pixel(0, 0)), 0);
  QCOMPARE(qAlpha(frame.pixel(frame.width() - 1, frame.height() - 1)), 0);
}

void TestExportImage::checkTransparentGrabIsRestoredAfterwards()
{
  restoreWindowInitialState();

  window->qglview->grabFrame(true);

  // A transparent grab must not leave the on-screen view transparent: the alpha-scrubbing pass in
  // GLView::paintGL exists to stop Qt compositing the viewport's alpha (issue #3689), and skipping
  // it permanently would make the window itself go see-through on wayland.
  QVERIFY(!window->qglview->transparentBackground());

  const QImage frame = window->qglview->grabFrame(false);
  QCOMPARE(qAlpha(frame.pixel(0, 0)), 255);
}

void TestExportImage::checkRepeatedGrabsDoNotAccumulate()
{
  restoreWindowInitialState();

  // Transparent frames must not composite onto the previous frame. GLView::paintGL clears the
  // color buffer unconditionally at the top, and QOpenGLWidget defaults to NoPartialUpdate, so
  // nothing should carry over -- but accumulation is invisible until it isn't, and it would show up
  // as alpha creeping toward opaque over successive grabs.
  const QImage first = window->qglview->grabFrame(true).copy();
  const QImage second = window->qglview->grabFrame(true).copy();

  QCOMPARE(first.size(), second.size());
  QCOMPARE(second, first);

  // Interleaving an opaque grab must not contaminate the next transparent one either.
  window->qglview->grabFrame(false);
  const QImage third = window->qglview->grabFrame(true).copy();
  QCOMPARE(third, first);
  QCOMPARE(qAlpha(third.pixel(0, 0)), 0);
}

void TestExportImage::checkCompositingKeepsTheDefaultGrabOpaqueAndUnchanged()
{
  restoreWindowInitialState();

  // With transparent-compositing enabled the view is rendered onto a transparent buffer and the
  // background is composited underneath at the end of paintGL. An ordinary (non-transparent) grab
  // must therefore still come back opaque and looking exactly as before -- the failure this guards
  // against is grabbing the pre-composite transparent frame and flattening it to RGB, which would
  // silently darken everything toward black.
  const QImage before = window->qglview->grabFrame(false).copy();

  Feature::enable_feature("transparent-compositing", true);
  const QImage after = window->qglview->grabFrame(false).copy();
  Feature::enable_feature("transparent-compositing", false);

  QCOMPARE(after.size(), before.size());
  QCOMPARE(qAlpha(after.pixel(0, 0)), 255);
  QCOMPARE(qAlpha(after.pixel(after.width() / 2, after.height() / 2)), 255);

  // Allow only 8-bit rounding through the premultiplied intermediate.
  int worst = 0;
  for (int y = 0; y < before.height(); ++y) {
    for (int x = 0; x < before.width(); ++x) {
      const QRgb a = before.pixel(x, y);
      const QRgb b = after.pixel(x, y);
      worst = std::max({worst, std::abs(qRed(a) - qRed(b)), std::abs(qGreen(a) - qGreen(b)),
                        std::abs(qBlue(a) - qBlue(b))});
    }
  }
  QVERIFY2(worst <= 2, qPrintable(QString("compositing changed the ordinary grab by %1").arg(worst)));
}

namespace {

// restoreWindowInitialState() opens the file and returns; it does not wait for the preview to be
// built. The other tests here only assert on background corners, so they pass whether or not the
// model has appeared -- this one compares whole frames, so it has to wait for the render to
// settle first. Settled means two successive grabs agree.
QImage settledTransparentFrame(QGLView *view)
{
  // Settled means several consecutive grabs agree, not just two: an async render that has not
  // started yet produces a run of identical empty frames, which a two-frame check accepts.
  QImage previous = view->grabFrame(true).copy();
  int stable = 0;
  for (int i = 0; i < 200; ++i) {
    qApp->processEvents();
    QTest::qWait(50);
    QImage current = view->grabFrame(true).copy();
    stable = (current == previous) ? stable + 1 : 0;
    previous = std::move(current);
    if (stable >= 8) break;
  }
  return previous;
}

}  // namespace

void TestExportImage::checkGrabAfterAModalDialogIsStillTransparent()
{
  restoreWindowInitialState();

  // The Export Image flow asks for transparency through the save panel's format popup, so the
  // frame can only be grabbed once that panel has closed -- it used to be grabbed beforehand,
  // guarded by a comment about keeping the dialog out of the image. grabFramebuffer() repaints
  // into its own framebuffer object rather than reading the screen, so nothing on screen can
  // reach the image and the order is free. This pins that: a grab taken after a modal dialog has
  // come and gone must be byte-identical to one taken before it.
  const QImage before = settledTransparentFrame(window->qglview);

  QDialog modal(window);
  modal.setModal(true);
  modal.show();
  QVERIFY(QTest::qWaitForWindowExposed(&modal));
  modal.close();
  qApp->processEvents();

  const QImage after = window->qglview->grabFrame(true).copy();

  QCOMPARE(after.size(), before.size());
  QCOMPARE(qAlpha(after.pixel(0, 0)), 0);
  QCOMPARE(after, before);
}

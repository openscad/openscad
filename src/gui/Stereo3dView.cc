#include "gui/Stereo3dView.h"

#include <QString>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)

// QGLView.h first: the OpenGL loader must come before the Qt OpenGL headers
#include "gui/QGLView.h"
#include "core/Settings.h"

#include <QOpenGLWidget>
#include <QSettings>
#include <QSurfaceFormat>

Stereo3dView::Stereo3dView(QGLView& view) : view(view), stereo(Settings::Settings::stereo3dView.value())
{
}

void Stereo3dView::requestStereoBuffers(QSurfaceFormat& format)
{
  // QOpenGLWidget draws for each eye only if the default format asks for stereo buffers, and that
  // must be known before the window is created, so the preference is read from the settings here.
  const QSettings settings("OpenSCAD", "OpenSCAD");
  if (settings.value(QString::fromStdString(Settings::Settings::stereo3dView.key()), false).toBool()) {
    format.setOption(QSurfaceFormat::StereoBuffers);
  }
}

bool Stereo3dView::drawsEyes() const
{
  return stereo && view.format().stereo();
}

void Stereo3dView::beginEye()
{
  // One eye per call: the left buffer first, then the right one
  if (!drawsEyes()) return;
  const auto halfSeparation =
    Settings::Settings::stereo3dEyeSeparation.value() / 100.0 * view.cam.zoomValue() / 2.0;
  view.stereoEyeOffset =
    view.currentTargetBuffer() == QOpenGLWidget::RightBuffer ? halfSeparation : -halfSeparation;
}

void Stereo3dView::endEye()
{
  if (!drawsEyes()) return;
  depthEyeOffset = view.stereoEyeOffset;
  view.stereoEyeOffset = 0.0;
  view.setupCamera();
}

void Stereo3dView::beginDepthRead()
{
  // The depth buffer holds the eye drawn last
  if (!drawsEyes()) return;
  view.stereoEyeOffset = depthEyeOffset;
}

void Stereo3dView::endDepthRead()
{
  if (!drawsEyes()) return;
  view.stereoEyeOffset = 0.0;
  view.setupCamera();
}

#else  // Qt before 6.5: the stereo 3D view is not available

Stereo3dView::Stereo3dView(QGLView& /*view*/)
{
}

void Stereo3dView::requestStereoBuffers(QSurfaceFormat& /*format*/)
{
}

void Stereo3dView::beginEye()
{
}

void Stereo3dView::endEye()
{
}

void Stereo3dView::beginDepthRead()
{
}

void Stereo3dView::endDepthRead()
{
}

#endif  // QT_VERSION

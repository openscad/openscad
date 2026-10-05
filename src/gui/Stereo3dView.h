#pragma once

#include <QtGlobal>

class QGLView;
class QSurfaceFormat;

/*
 * The stereo 3D view. With the "stereo 3D view" preference on and an OpenGL context with stereo
 * buffers, QOpenGLWidget calls QGLView::paintGL() once for the left buffer and once for the right
 * buffer, and this class sets the camera of each eye through GLView::stereoEyeOffset.
 *
 * It needs QOpenGLWidget::currentTargetBuffer(), which came with Qt 6.5. With older Qt this class
 * is a stub: it never asks for stereo buffers and never moves the eye, so the eye offset stays 0
 * and the 3D view is the normal single view. This class is the only place where the stereo 3D view
 * depends on the Qt version.
 */
class Stereo3dView
{
public:
  explicit Stereo3dView(QGLView& view);

  // Asks for stereo buffers in the default surface format when the preference is on
  static void requestStereoBuffers(QSurfaceFormat& format);

  // Around GLView::paintGL(): sets the offset of the eye drawn now, then sets up the camera between
  // the eyes again, which picking uses
  void beginEye();
  void endEye();
  // Around reading the depth buffer: sets the offset of the eye drawn last, whose depth the buffer
  // holds, then sets up the camera between the eyes again
  void beginDepthRead();
  void endDepthRead();

private:
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  bool drawsEyes() const;

  QGLView& view;
  bool stereo;                  // the preference, as it was when the view was created
  double depthEyeOffset = 0.0;  // eye offset of the view the depth buffer holds
#endif
};

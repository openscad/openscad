/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "gui/QGLView.h"
#include "glview/glthread-context.h"
#include "glview/system-gl.h"
#ifdef USE_GLAD
#include <glad/gl.h>
#endif
#include <cmath>
#include <memory>
#include <QtCore/qpoint.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLFunctions>
#endif

#include "core/Selection.h"
#include "geometry/linalg.h"
#include "gui/qtgettext.h"
#include "gui/Preferences.h"
#include "glview/Renderer.h"
#include "utils/degree_trig.h"
#include "utils/scope_guard.hpp"
#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
#include "glview/glew-utils.h"
#endif

#include <QImage>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include <QWidget>
#include <QPainter>
#include <iostream>
#include <QApplication>
#include <QWheelEvent>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QErrorMessage>
#ifdef USE_GLAD
#include <QOpenGLContext>
#endif
#include "gui/OpenCSGWarningDialog.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
#ifdef __APPLE__
#include <dlfcn.h>
#endif

#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif

#include "gui/qt-obsolete.h"
#include "gui/Measurement.h"

namespace {

QSurfaceFormat compatibleWidgetFormat()
{
  auto format = QSurfaceFormat::defaultFormat();
  format.setRenderableType(QSurfaceFormat::OpenGL);
  format.setProfile(QSurfaceFormat::CompatibilityProfile);
  if (format.depthBufferSize() < 24) format.setDepthBufferSize(24);
  if (format.stencilBufferSize() < 8) format.setStencilBufferSize(8);
  return format;
}

}  // namespace

QGLView::QGLView(QWidget *parent) : QOpenGLWidget(parent)
{
  setFormat(compatibleWidgetFormat());
  init();
}

QGLView::~QGLView()
{
  // Just to make sure we can call GL functions in the supertype destructor
  makeCurrent();
}

void QGLView::init()
{
  resetView();

  this->mouse_drag_active = false;
  this->statusLabel = nullptr;

  setMouseTracking(true);
}

void QGLView::resetView()
{
  cam.resetView();
}

void QGLView::viewAll()
{
  if (auto renderer = this->getRenderer()) {
    auto bbox = renderer->getBoundingBox();
    cam.autocenter = true;
    cam.viewAll(renderer->getBoundingBox());
  }
}

void QGLView::initializeGL()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  // Qt6: Initialize QOpenGLFunctions first to help GLAD find GL functions
  // This helps with GLAD initialization on macOS where Qt6's getProcAddress is unreliable
  std::cerr << "Qt6 detected: Initializing QOpenGLFunctions first" << std::endl;

  // Create GL functions object for this context
  auto gl_funcs = std::make_unique<QOpenGLFunctions>();

  // Initialize the GL functions with current context
  gl_funcs->initializeOpenGLFunctions();
  std::cerr << "Successfully initialized QOpenGLFunctions" << std::endl;

  // Keep the GL functions object in case we need it later
  this->gl_functions = std::move(gl_funcs);
#endif

  // Use GLAD or GLEW on all platforms (Qt5 and Qt6)
#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
  // Since OpenCSG requires glew, we need to initialize it.
  // ..in a separate compilation unit to avoid duplicate symbols with x.
  initializeGlew();
#endif
#ifdef USE_GLAD
  // On Qt6/macOS, use Qt's getProcAddress for GLAD initialization
  // Qt should properly handle both GL and GLU function resolution
  std::cerr << "GLAD initialization via Qt getProcAddress..." << std::endl;

  const auto version = gladLoadGLUserPtr(
    [](void *ctx, const char *name) -> GLADapiproc {
      auto addr = reinterpret_cast<QOpenGLContext *>(ctx)->getProcAddress(name);
      return addr;
    },
    this->context());

  this->glad_available = (version != 0);

  if (this->glad_available) {
    std::cerr << "GLAD successfully loaded via Qt, version: " << version << std::endl;
  } else {
    std::cerr << "GLAD initialization via Qt failed, trying direct framework loading..." << std::endl;

#ifdef __APPLE__
    // Fallback: Load GL functions directly from OpenGL framework on macOS
    void* gl_lib = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY);
    if (gl_lib) {
      std::cerr << "Trying direct OpenGL framework load as fallback..." << std::endl;

      const auto fw_version = gladLoadGLUserPtr(
        [](void *ctx, const char *name) -> GLADapiproc {
          void* gl_lib = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY);
          if (!gl_lib) return nullptr;
          void* sym = dlsym(gl_lib, name);
          return reinterpret_cast<GLADapiproc>(sym);
        },
        this->context());

      this->glad_available = (fw_version != 0);
      std::cerr << "Framework direct load result: " << fw_version << std::endl;
      dlclose(gl_lib);
    }
#endif

    if (!this->glad_available) {
      std::cerr << "GLAD initialization completely failed" << std::endl;
    }
  }

  // On macOS/Qt6, GLAD initialization fails but we can still try basic GL operations
  // through Qt's OpenGL API. We'll skip GLAD initialization entirely in favor of
  // safer Qt functions, but still allow gl_dump() and basic operations
  if (!this->glad_available && this->context()) {
    std::cerr << "GLAD failed but Qt context available - using fallback rendering" << std::endl;
    // Don't set glad_available=true here; stick with fallback rendering instead
    // This is safer than trying to use uninitialized function pointers
  }
#else
  this->glad_available = true;
#endif  // ifdef USE_GLAD

  // Only try GL operations if initialization was successful
  if (this->glad_available) {
    try {
      PRINTD(gl_dump());

      // Cache renderer info for the OpenCSG warning dialog
      // This must happen in initializeGL() when GL context is active
      // Note: We skip caching GL strings to avoid compile-time issues with GLAD
      // The dialog will display "(Renderer information not available)" which is acceptable
    } catch (...) {
      std::cerr << "Warning: GL dump failed" << std::endl;
    }

    try {
      GLView::initializeGL();
    } catch (...) {
      std::cerr << "Warning: GLView::initializeGL() failed" << std::endl;
    }
  } else {
    std::cerr << "Skipping GL initialization (GL not available)" << std::endl;
  }

  std::cerr << "Creating MouseSelector..." << std::endl;
  try {
    this->selector = std::make_unique<MouseSelector>(this);
    std::cerr << "MouseSelector created successfully" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: MouseSelector creation failed: " << e.what() << std::endl;
    return;
  } catch (...) {
    std::cerr << "ERROR: MouseSelector creation failed with unknown exception" << std::endl;
    return;
  }

  std::cerr << "Emitting initialized signal..." << std::endl;
  emit initialized();
  std::cerr << "QGLView::initializeGL() COMPLETE!" << std::endl;
}

std::string QGLView::getRendererInfo() const
{
  std::ostringstream info;
  info << gl_dump();
  // Don't translate as translated text in the Library Info dialog is not wanted
  info << "\nQt graphics widget: QOpenGLWidget";
  auto qsf = this->format();
  auto rbits = qsf.redBufferSize();
  auto gbits = qsf.greenBufferSize();
  auto bbits = qsf.blueBufferSize();
  auto abits = qsf.alphaBufferSize();
  auto dbits = qsf.depthBufferSize();
  auto sbits = qsf.stencilBufferSize();
  info << boost::format("\nQSurfaceFormat: RGBA(%d%d%d%d), depth(%d), stencil(%d)\n\n") % rbits % gbits %
            bbits % abits % dbits % sbits;
  info << gl_extensions_dump();
  return info.str();
}

#ifdef ENABLE_OPENCSG
void QGLView::display_opencsg_warning()
{
  if (GlobalPreferences::inst()->getValue("advanced/opencsg_show_warning").toBool()) {
    QTimer::singleShot(0, this, &QGLView::display_opencsg_warning_dialog);
  }
}

void QGLView::display_opencsg_warning_dialog()
{
  auto dialog = new OpenCSGWarningDialog(this);

  QString message =
    _("Warning: Missing OpenGL capabilities for OpenCSG - OpenCSG has been disabled.\n\n");
  message +=
    _("It is highly recommended to use OpenSCAD on a system with "
      "OpenGL 2.0 or later.\n"
      "Your renderer information is as follows:\n");

  // Use cached renderer info from initializeGL() to avoid GL calls outside render context
  if (!this->cached_renderer_info.empty()) {
    message += QString::fromStdString(this->cached_renderer_info);
  } else {
    message += _("(Renderer information not available)");
  }
  dialog->setText(message);
  dialog->exec();
}
#endif  // ifdef ENABLE_OPENCSG

void QGLView::resizeGL(int w, int h)
{
  // Skip GL operations if GLAD isn't available or context isn't available (Qt6/macOS)
  if (!this->glad_available || !this->context()) return;

  // Set thread-local GL context if using Qt6 QOpenGLFunctions
  if (this->gl_functions) {
    setCurrentGLFunctions(this->gl_functions.get());
  }

  try {
    GLView::resizeGL(w, h);
  } catch (...) {
    std::cerr << "Warning: GLView::resizeGL() failed" << std::endl;
  }
  emit resized();
}

void QGLView::paintGL()
{
  if (this->glad_available && this->context()) {
    // Set thread-local GL context if using Qt6 QOpenGLFunctions
    if (this->gl_functions) {
      setCurrentGLFunctions(this->gl_functions.get());
    }

    try {
      GLView::paintGL();
    } catch (...) {
      std::cerr << "Warning: GLView::paintGL() failed" << std::endl;
    }
  } else {
    // Fallback: render a yellow background using QPainter when GL is unavailable
    // This at least provides visual feedback that the app is running
    std::cerr << "Using fallback rendering (no GL available)" << std::endl;
    QPainter painter(this);
    painter.fillRect(rect(), QColor(255, 255, 0));  // Yellow background
    painter.drawText(rect(), Qt::AlignCenter, "OpenGL unavailable");
    painter.end();
  }

  if (statusLabel) {
    auto status = QString("%1 (%2x%3)")
                    .arg(QString::fromStdString(cam.statusText()))
                    .arg(size().rwidth())
                    .arg(size().rheight());
    statusLabel->setText(status);
  }
}

void QGLView::mousePressEvent(QMouseEvent *event)
{
  if (!mouse_drag_active) {
    mouse_drag_moved = false;
  }

  mouse_drag_active = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  last_mouse = event->globalPosition();
#else
  last_mouse = event->globalPos();
#endif
}

/*
 * Voodoo warning...
 *
 * This function selects the widget's OpenGL context (via this->makeCurrent()).
 * Because it's changing the OpenGL context, it seems polite to save and restore it.
 * That resolution seems correct, independent of the mysteries below.
 *
 * Let's call the widget's context W, and the alternate context that we are called with A.
 *
 * It's important that A is selected when we return (as it is when we enter), because
 * if it isn't then sometimes the subsequent mouseReleaseEvent is called with W, when it
 * is normally called with A.  When that happens, the object-selection magic in selectObject
 * messes up W, and rendering is forever after broken in that window.
 *
 * However, as hygienic as saving-and-restoring seems, the picture is still unsatisfying.
 *
 * Open questions:
 * - Why are these mouse event functions called with A, rather than being called with W?
 *   It's unsurprising that the selection magic needs its own GL context, but it seems like
 *   it should be the one that needs to explicitly select it, not this function.
 * - Where did A come from?
 * - Why does a subsequent mouseReleaseEvent call get called with W?
 * - Why does it only sometimes get called with W, and sometimes (correctly) with A?
 * - Why do later mouseReleaseEvent calls revert to being (correctly) called with A?
 * - Why does this only happen with right clicks?  With left clicks, this function
 *   changes the context, but it's OK again on the following mouseReleaseEvent.
 * - Why does this only happen when you click on empty space, and not when you click
 *   on the model?  Double clicks on the model are not detected as double clicks.
 *   Perhaps this is because the first click pops a menu and the second click is
 *   on the menu, not this widget.
 *
 * getGLContext() and setGLContext() are in a separate file, QGLView2.cc, so that this
 * file doesn't need a full declaration of QOpenGLContext.  <QOpenGLContext> is
 * incompatible with GLEW and causes compilation warnings.
 *
 * For future attention:
 * - This function should probably only react to left double clicks.  Right double clicks
 *   should probably be ignored.
 */
void QGLView::mouseDoubleClickEvent(QMouseEvent *event)
{
  QOpenGLContext *oldContext = getGLContext();
  this->makeCurrent();
  auto guard = sg::make_scope_guard([this, oldContext] {
    this->doneCurrent();
    setGLContext(oldContext);
  });

  setupCamera();

#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
  // Center View feature only works with GLEW (GLAD on Qt6/macOS has issues with these GL calls)
  int viewport[4];
  GLdouble modelview[16];
  GLdouble projection[16];

  glGetIntegerv(GL_VIEWPORT, viewport);
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  glGetDoublev(GL_PROJECTION_MATRIX, projection);

  const double dpi = this->getDPI();
  const double x = event->pos().x() * dpi;
  const double y = viewport[3] - event->pos().y() * dpi;
  GLfloat z = 0;

  glGetError();  // clear error state so we don't pick up previous errors
  glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
  if (const auto glError = glGetError(); glError != GL_NO_ERROR) {
    if (statusLabel) {
      auto status = QString("Center View: OpenGL Error reading Pixel: %s")
                      .arg(QString::fromLocal8Bit((const char *)gluErrorString(glError)));
      statusLabel->setText(status);
    }
    return;
  }

  if (z == 1) {
    return;  // outside object
  }

  GLdouble px, py, pz;

  auto success = gluUnProject(x, y, z, modelview, projection, viewport, &px, &py, &pz);

  if (success == GL_TRUE) {
    cam.object_trans -= Vector3d(px, py, pz);
    update();
    emit cameraChanged();
  }
#endif  // USE_GLEW || OPENCSG_GLEW
}

void QGLView::normalizeAngle(GLdouble& angle)
{
  while (angle < 0) angle += 360;
  while (angle > 360) angle -= 360;
}

void QGLView::mouseMoveEvent(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  auto this_mouse = event->globalPosition();
#else
  auto this_mouse = event->globalPos();
#endif
  if (measure_state != Measurement::MEASURE_IDLE) {
    QPoint pt = event->pos();
    this->shown_obj = findObject(pt.x(), pt.y());
    update();
  }
  double dx = (this_mouse.x() - last_mouse.x()) * 0.7;
  double dy = (this_mouse.y() - last_mouse.y()) * 0.7;
  if (mouse_drag_active) {
    mouse_drag_moved = true;

    bool multipleButtonsPressed = false;
    int buttonIndex = -1;
    if (event->buttons() & Qt::LeftButton) {
      buttonIndex = 0;
    }
    if (event->buttons() & Qt::MiddleButton) {
      if (buttonIndex != -1) {
        multipleButtonsPressed = true;
      } else {
        buttonIndex = 1;
      }
    }
    if (event->buttons() & Qt::RightButton) {
      if (buttonIndex != -1) {
        multipleButtonsPressed = true;
      } else {
        buttonIndex = 2;
      }
    }
    int modifierIndex = 0;
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
      modifierIndex = 1;
    }
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
      if (modifierIndex == 1) {
        modifierIndex = 3;  // Ctrl + Shift
      } else {
        modifierIndex = 2;
      }
    }

    if (buttonIndex != -1 && !multipleButtonsPressed) {
      float *selectedMouseActions =
        &this->mouseActions[MouseConfig::ACTION_DIMENSION * (buttonIndex + modifierIndex * 3)];

      // Rotation angles from mouse movement
      // First 6 elements to selectedMouseActions are interpreted as a row-major 3x2 matrix, which is
      // right-multiplied by (dx, dy)^T to produce the rotation angle increments.
      double rx = selectedMouseActions[0] * dx + selectedMouseActions[1] * dy;
      double ry = selectedMouseActions[2] * dx + selectedMouseActions[3] * dy;
      double rz = selectedMouseActions[4] * dx + selectedMouseActions[5] * dy;
      if (!(rx == 0.0 && ry == 0.0 && rz == 0.0)) {
        rotate(rx, ry, rz, true);
        normalizeAngle(cam.object_rot.x());
        normalizeAngle(cam.object_rot.y());
        normalizeAngle(cam.object_rot.z());
      }

      // Panning from mouse movement
      // Elements 6..12 of selectedMouseActions are interpreted as another row-major 3x2 matrix, which is
      // right-multiplied by (dx, dy)^T, and then scaled by the zoom, to produce the translation
      // increments.
      double mx = selectedMouseActions[6 + 0] * (dx / QWidget::width()) +
                  selectedMouseActions[6 + 1] * (dy / QWidget::height());
      double my = selectedMouseActions[6 + 2] * (dx / QWidget::width()) +
                  selectedMouseActions[6 + 3] * (dy / QWidget::height());
      double mz = selectedMouseActions[6 + 4] * (dx / QWidget::width()) +
                  selectedMouseActions[6 + 5] * (dy / QWidget::height());
      if (!(mx == 0.0 && my == 0.0 && mz == 0.0)) {
        mx *= 3.0 * cam.zoomValue();
        my *= 3.0 * cam.zoomValue();
        mz *= 3.0 * cam.zoomValue();
      }
      translate(mx, my, mz, true);

      // Zoom from mouse movement
      // Final 2 elements of selectedMouseActions are interpreted as a 2-dimensional vector. The inner
      // product of this is taken with (dx, dy)^T to produce the zoom increment.
      double dZoom = selectedMouseActions[12] * dx + selectedMouseActions[13] * dy;
      if (dZoom != 0.0) {
        dZoom *= 12.0;
        zoom(dZoom, true);
      }
    }
  }
  last_mouse = this_mouse;
}

void QGLView::mouseReleaseEvent(QMouseEvent *event)
{
  mouse_drag_active = false;
  releaseMouse();

  if (!mouse_drag_moved) {
    if (event->button() == Qt::RightButton) {
      QPoint point = event->pos();
      emit doRightClick(point);
    }
    if (event->button() == Qt::LeftButton) {
      QPoint point = event->pos();
      emit doLeftClick(point);
    }
  }
  mouse_drag_moved = false;
}

const QImage& QGLView::grabFrame()
{
  // Force reading from front buffer. Some configurations will read from the back buffer here.
  glReadBuffer(GL_FRONT);
  this->frame = grabFramebuffer();
  return this->frame;
}

bool QGLView::save(const char *filename) const
{
  return this->frame.save(filename, "PNG");
}

void QGLView::wheelEvent(QWheelEvent *event)
{
  const auto pos = Q_WHEEL_EVENT_POSITION(event);
  const int v = event->angleDelta().y();
  if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
    zoomFov(v);
  } else if (this->mouseCentricZoom) {
    zoomCursor(pos.x(), pos.y(), v);
  } else {
    zoom(v, true);
  }
}

void QGLView::ZoomIn()
{
  zoom(120, true);
}

void QGLView::ZoomOut()
{
  zoom(-120, true);
}

void QGLView::zoom(double v, bool relative)
{
  this->cam.zoom(v, relative);
  update();
  emit cameraChanged();
}

void QGLView::zoomFov(double v)
{
  this->cam.setVpf(this->cam.fovValue() * pow(0.9, v / 120.0));
  update();
  emit cameraChanged();
}

void QGLView::zoomCursor(int x, int y, int zoom)
{
  const auto old_dist = cam.zoomValue();
  this->cam.zoom(zoom, true);
  const auto dist = cam.zoomValue();
  const auto ratio = old_dist / dist - 1.0;
  // screen coordinates from -1 to 1
  const auto screen_x = 2.0 * (x + 0.5) / this->cam.pixel_width - 1.0;
  const auto screen_y = 1.0 - 2.0 * (y + 0.5) / this->cam.pixel_height;
  const auto height = dist * tan_degrees(cam.fov / 2);
  const auto mx = ratio * screen_x * (aspectratio * height);
  const auto mz = ratio * screen_y * height;
  translate(-mx, 0, -mz, true);
}

void QGLView::setOrthoMode(bool enabled)
{
  if (enabled) this->cam.setProjection(Camera::ProjectionType::ORTHOGONAL);
  else this->cam.setProjection(Camera::ProjectionType::PERSPECTIVE);
}

void QGLView::translate(double x, double y, double z, bool relative, bool viewPortRelative)
{
  Matrix3d aax, aay, aaz;
  aax = angle_axis_degrees(-cam.object_rot.x(), Vector3d::UnitX());
  aay = angle_axis_degrees(-cam.object_rot.y(), Vector3d::UnitY());
  aaz = angle_axis_degrees(-cam.object_rot.z(), Vector3d::UnitZ());
  Matrix3d tm3 = aaz * aay * aax;

  Matrix4d tm = Matrix4d::Identity();
  if (viewPortRelative) {
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        tm(j, i) = tm3(j, i);
      }
    }
  }

  Matrix4d vec;
  // clang-format off
  vec << 0, 0, 0, x,
         0, 0, 0, y,
         0, 0, 0, z,
         0, 0, 0, 1;
  // clang-format on
  tm = tm * vec;
  double f = relative ? 1 : 0;
  cam.object_trans.x() = f * cam.object_trans.x() + tm(0, 3);
  cam.object_trans.y() = f * cam.object_trans.y() + tm(1, 3);
  cam.object_trans.z() = f * cam.object_trans.z() + tm(2, 3);
  update();
  emit cameraChanged();
}

void QGLView::rotate(double x, double y, double z, bool relative)
{
  double f = relative ? 1 : 0;
  cam.object_rot.x() = f * cam.object_rot.x() + x;
  cam.object_rot.y() = f * cam.object_rot.y() + y;
  cam.object_rot.z() = f * cam.object_rot.z() + z;
  normalizeAngle(cam.object_rot.x());
  normalizeAngle(cam.object_rot.y());
  normalizeAngle(cam.object_rot.z());
  update();
  emit cameraChanged();
}

void QGLView::rotate2(double x, double y, double z)
{
  // This vector describes the rotation.
  // The direction of the vector is the angle around which to rotate, and
  // the length of the vector is the angle by which to rotate
  Vector3d rot = Vector3d(-x, -y, -z);

  // get current rotation matrix
  Matrix3d aax, aay, aaz, rmx;
  aax = angle_axis_degrees(-cam.object_rot.x(), Vector3d::UnitX());
  aay = angle_axis_degrees(-cam.object_rot.y(), Vector3d::UnitY());
  aaz = angle_axis_degrees(-cam.object_rot.z(), Vector3d::UnitZ());
  rmx = aaz * (aay * aax);

  // rotate
  rmx = rmx * angle_axis_degrees(rot.norm(), rot.normalized());

  // back to euler
  // see: http://staff.city.ac.uk/~sbbh653/publications/euler.pdf
  double theta, psi, phi;
  if (abs(rmx(2, 0)) != 1) {
    theta = -asin_degrees(rmx(2, 0));
    psi = atan2_degrees(rmx(2, 1) / cos_degrees(theta), rmx(2, 2) / cos_degrees(theta));
    phi = atan2_degrees(rmx(1, 0) / cos_degrees(theta), rmx(0, 0) / cos_degrees(theta));
  } else {
    phi = 0;
    if (rmx(2, 0) == -1) {
      theta = 90;
      psi = phi + atan2_degrees(rmx(0, 1), rmx(0, 2));
    } else {
      theta = -90;
      psi = -phi + atan2_degrees(-rmx(0, 1), -rmx(0, 2));
    }
  }

  cam.object_rot.x() = -psi;
  cam.object_rot.y() = -theta;
  cam.object_rot.z() = -phi;

  normalizeAngle(cam.object_rot.x());
  normalizeAngle(cam.object_rot.y());
  normalizeAngle(cam.object_rot.z());

  update();
  emit cameraChanged();
}

std::vector<SelectedObject> QGLView::findObject(int mouse_x, int mouse_y)
{
  int viewport[4] = {0, 0, 0, 0};
  double posXF, posYF, posZF;
  double posXN, posYN, posZN;
  viewport[2] = size().rwidth();
  viewport[3] = size().rheight();

  GLdouble winX = mouse_x;
  GLdouble winY = viewport[3] - mouse_y;

  gluUnProject(winX, winY, 1, this->modelview, this->projection, viewport, &posXF, &posYF, &posZF);
  gluUnProject(winX, winY, -1, this->modelview, this->projection, viewport, &posXN, &posYN, &posZN);
  Vector3d far_pt(posXF, posYF, posZF);
  Vector3d near_pt(posXN, posYN, posZN);

  Vector3d testpt(0, 0, 0);
  std::vector<SelectedObject> result;
  auto renderer = this->getRenderer();
  if (renderer == nullptr) return result;
  result = renderer->findModelObject(near_pt, far_pt, mouse_x, mouse_y, cam.zoomValue() / 300);
  return result;
}

void QGLView::selectPoint(int mouse_x, int mouse_y)
{
  std::vector<SelectedObject> obj = findObject(mouse_x, mouse_y);
  if (obj.size() == 1) {
    this->selected_obj.push_back(obj[0]);
    update();
  }
}

int QGLView::pickObject(QPoint position)
{
  if (!isValid()) return -1;

  if (this->getRenderer()) {
    this->makeCurrent();
    auto guard = sg::make_scope_guard([this]() { this->doneCurrent(); });

    // Update the selector with the right image size
    this->selector->reset(this);

    return this->selector->select(this->getRenderer(), position.x(), position.y());
  }
  return -1;
}

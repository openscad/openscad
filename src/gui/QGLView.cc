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

#include "gui/qtgettext.h"
#include "gui/Preferences.h"
#include "glview/Renderer.h"
#include "utils/degree_trig.h"
#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
#include "glview/glew-utils.h"
#endif

#include <QImage>
#include <QOpenGLWidget>
#include <QWidget>
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

#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

#include "gui/qt-obsolete.h"
#include "gui/Measurement.h"

QGLView::QGLView(QWidget *parent) : QOpenGLWidget(parent)
{
  init();
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
#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
  // Since OpenCSG requires glew, we need to initialize it.
  // ..in a separate compilation unit to avoid duplicate symbols with GLAD.
  initializeGlew();
#endif
#ifdef USE_GLAD
  // We could ask for gladLoadGLES2UserPtr() here if we want to use GLES2+
  const auto version = gladLoadGLUserPtr([](void *ctx, const char *name) -> GLADapiproc {
    return reinterpret_cast<QOpenGLContext *>(ctx)->getProcAddress(name);
  }, this->context());
  if (version == 0) {
    std::cerr << "Unable to init GLAD" << std::endl;
    return;
  }
  PRINTDB("GLAD: Loaded OpenGL %d.%d", GLAD_VERSION_MAJOR(version) % GLAD_VERSION_MINOR(version));
#endif // ifdef USE_GLAD
  GLView::initializeGL();
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
  info << boost::format("\nQSurfaceFormat: RGBA(%d%d%d%d), depth(%d), stencil(%d)\n\n") %
    rbits % gbits % bbits % abits % dbits % sbits;
  info << gl_extensions_dump();
  return info.str();
}

#ifdef ENABLE_OPENCSG
void QGLView::display_opencsg_warning()
{
  if (Preferences::inst()->getValue("advanced/opencsg_show_warning").toBool()) {
    QTimer::singleShot(0, this, SLOT(display_opencsg_warning_dialog()));
  }
}

void QGLView::display_opencsg_warning_dialog()
{
  auto dialog = new OpenCSGWarningDialog(this);

  QString message = _("Warning: Missing OpenGL capabilities for OpenCSG - OpenCSG has been disabled.\n\n");
  message += _("It is highly recommended to use OpenSCAD on a system with "
               "OpenGL 2.0 or later.\n"
               "Your renderer information is as follows:\n");
#if defined(USE_GLEW) || defined(OPENCSG_GLEW)
  QString rendererinfo(_("GLEW version %1\n%2 (%3)\nOpenGL version %4\n"));
  message += rendererinfo.arg((const char *)glewGetString(GLEW_VERSION),
                              (const char *)glGetString(GL_RENDERER),
                              (const char *)glGetString(GL_VENDOR),
                              (const char *)glGetString(GL_VERSION));
#endif
#ifdef USE_GLAD
  QString rendererinfo(_("GLAD version %1\n%2 (%3)\nOpenGL version %4\n"));
  message += rendererinfo.arg(GLAD_GENERATOR_VERSION,
                              (const char *)glGetString(GL_RENDERER),
                              (const char *)glGetString(GL_VENDOR),
                              (const char *)glGetString(GL_VERSION));
#endif
  dialog->setText(message);
  dialog->exec();
}
#endif // ifdef ENABLE_OPENCSG

void QGLView::resizeGL(int w, int h)
{
  GLView::resizeGL(w, h);
  emit resized();
}

void QGLView::paintGL()
{
  GLView::paintGL();

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
  last_mouse = event->globalPos();
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
void QGLView::mouseDoubleClickEvent(QMouseEvent *event) {
  QOpenGLContext *oldContext = getGLContext();
  this->makeCurrent();
  setupCamera();

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

  glGetError(); // clear error state so we don't pick up previous errors
  glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
  if (const auto glError = glGetError(); glError != GL_NO_ERROR) {
    if (statusLabel) {
      auto status = QString("Center View: OpenGL Error reading Pixel: %s")
        .arg(QString::fromLocal8Bit((const char *)gluErrorString(glError)));
      statusLabel->setText(status);
    }
    setGLContext(oldContext);
    return;
  }

  if (z == 1) {
    setGLContext(oldContext);
    return; // outside object
  }

  GLdouble px, py, pz;

  auto success = gluUnProject(x, y, z, modelview, projection, viewport, &px, &py, &pz);

  if (success == GL_TRUE) {
    cam.object_trans -= Vector3d(px, py, pz);
    update();
    emit cameraChanged();
  }
  setGLContext(oldContext);
}

void QGLView::normalizeAngle(GLdouble& angle)
{
  while (angle < 0) angle += 360;
  while (angle > 360) angle -= 360;
}

void QGLView::mouseMoveEvent(QMouseEvent *event)
{
  auto this_mouse = event->globalPos();
  if(measure_state != MEASURE_IDLE) {
	QPoint pt = event->pos();
  	this->shown_obj = findObject(pt.x(), pt.y());
	update();
  }
  double dx = (this_mouse.x() - last_mouse.x()) * 0.7;
  double dy = (this_mouse.y() - last_mouse.y()) * 0.7;
  if (mouse_drag_active) {
    mouse_drag_moved = true;
    auto button_compare = this->mouseSwapButtons?Qt::RightButton : Qt::LeftButton;
    if (event->buttons() & button_compare
#ifdef Q_OS_MACOS
        && !(event->modifiers() & Qt::MetaModifier)
#endif
        ) {
      // Left button rotates in xz, Shift-left rotates in xy
      // On Mac, Ctrl-Left is handled as right button on other platforms
      if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
        rotate(dy, dx, 0.0, true);
      } else {
        rotate(dy, 0.0, dx, true);
      }

      normalizeAngle(cam.object_rot.x());
      normalizeAngle(cam.object_rot.y());
      normalizeAngle(cam.object_rot.z());
    } else {
      // Right button pans in the xz plane
      // Middle button pans in the xy plane
      // Shift-right and Shift-middle zooms
      if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
        zoom(-12.0 * dy, true);
      } else {
        double mx = +(dx) * 3.0 * cam.zoomValue() / QWidget::width();
        double mz = -(dy) * 3.0 * cam.zoomValue() / QWidget::height();
        double my = 0;
        if (event->buttons() & Qt::MiddleButton) {
          my = mz;
          mz = 0;
          // actually lock the x-position
          // (turns out to be easier to use than xy panning)
          mx = 0;
        }

        translate(mx, my, mz, true);
      }
    }
  }
  last_mouse = this_mouse;
}

void QGLView::mouseReleaseEvent(QMouseEvent *event)
{
  mouse_drag_active = false;
  releaseMouse();

  auto button_right = this->mouseSwapButtons?Qt::LeftButton : Qt::RightButton;
  auto button_left =  this->mouseSwapButtons?Qt::RightButton : Qt::LeftButton;
  if (!mouse_drag_moved) {
    if(event->button() == button_right) {
      QPoint point = event->pos();
      emit doRightClick(point);
    }
    if(event->button() == button_left) {
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
    zoomFov (v);
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
  this->cam.setVpf( this->cam.fovValue () * pow(0.9, v / 120.0));
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
  vec <<
    0, 0, 0, x,
    0, 0, 0, y,
    0, 0, 0, z,
    0, 0, 0, 1
  ;
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

std::vector<SelectedObject> QGLView::findObject(int mouse_x,int mouse_y)
{
  int viewport[4]={0,0,0,0};
  double posXF, posYF, posZF;
  double posXN, posYN, posZN;
  viewport[2]=size().rwidth();
  viewport[3]=size().rheight();

  GLdouble winX = mouse_x;
  GLdouble winY = viewport[3] - mouse_y;

  gluUnProject(winX, winY, 1, this->modelview, this->projection, viewport,&posXF, &posYF, &posZF);
  gluUnProject(winX, winY, -1, this->modelview, this->projection, viewport,&posXN, &posYN, &posZN);
  Vector3d far_pt(posXF, posYF, posZF);
  Vector3d near_pt(posXN, posYN, posZN);

  Vector3d testpt(0,0,0);
  std::vector<SelectedObject> result;
  auto renderer = this->getRenderer();
  if(renderer == nullptr) return result;
  result = renderer->findModelObject(near_pt, far_pt, mouse_x, mouse_y, cam.zoomValue()/300);
  return result;
}

void QGLView::selectPoint(int mouse_x, int mouse_y)
{
  std::vector<SelectedObject>  obj= findObject(mouse_x, mouse_y);
  if(obj.size() == 1) {
    this->selected_obj.push_back(obj[0]);
    update();
  }
}

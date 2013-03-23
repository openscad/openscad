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

#include "QGLView.h"
#include "Preferences.h"
#include "renderer.h"

#include <QApplication>
#include <QWheelEvent>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QErrorMessage>
#include "OpenCSGWarningDialog.h"

#include "mathc99.h"
#include <stdio.h>

#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

#ifdef __APPLE__
#include <objc/message.h>

// Qt 4.8.4 doesn't have retina support for QGLWidget, so we need to peek at
// its internals to make that work.  This will likely break when updating to
// a newer Qt version, but that version will hopefully have built-in support
// for retina OpenGL.
extern OSWindowRef qt_mac_window_for(const QWidget*); //qwidget_mac.mm
extern OSViewRef qt_mac_nativeview_for(const QWidget *); //qwidget_mac.mm
#endif

QGLView::QGLView(QWidget *parent) : QGLWidget(parent)
{
  init();
}

QGLView::QGLView(const QGLFormat & format, QWidget *parent) : QGLWidget(format, parent)
{
  init();
}

static bool running_under_wine = false;

void QGLView::init()
{
  cam.type = Camera::GIMBAL;
  cam.object_rot << 35, 0, -25;
  cam.object_trans << 0, 0, 0;
  cam.viewer_distance = 500;

  this->mouse_drag_active = false;
  this->statusLabel = NULL;

  setMouseTracking(true);

// see paintGL() + issue160 + wine FAQ
#ifdef _WIN32
#include <windows.h>
  HMODULE hntdll = GetModuleHandle(L"ntdll.dll");
  if (hntdll)
    if ( (void *)GetProcAddress(hntdll, "wine_get_version") )
      running_under_wine = true;
#endif
}

void QGLView::initializeGL()
{
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
  }
  GLView::initializeGL();
}

std::string QGLView::getRendererInfo() const
{
  std::string glewinfo = glew_dump();
  std::string glextlist = glew_extensions_dump();
  return glewinfo + std::string("\nUsing QGLWidget\n\n") + glextlist;
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
  OpenCSGWarningDialog *dialog = new OpenCSGWarningDialog(this);

  QString message;
  if (this->is_opencsg_capable) {
    message += "Warning: You may experience OpenCSG rendering errors.\n\n";
  }
  else {
    message += "Warning: Missing OpenGL capabilities for OpenCSG - OpenCSG has been disabled.\n\n";
    dialog->enableOpenCSGBox->hide();
  }
  message += "It is highly recommended to use OpenSCAD on a system with "
    "OpenGL 2.0 or later.\n"
    "Your renderer information is as follows:\n";
  QString rendererinfo;
  rendererinfo.sprintf("GLEW version %s\n"
                       "%s (%s)\n"
                       "OpenGL version %s\n",
                       glewGetString(GLEW_VERSION),
                       glGetString(GL_RENDERER), glGetString(GL_VENDOR),
                       glGetString(GL_VERSION));
  message += rendererinfo;

  dialog->setText(message);
  dialog->enableOpenCSGBox->setChecked(Preferences::inst()->getValue("advanced/enable_opencsg_opengl1x").toBool());
  dialog->exec();

  opencsg_support = this->is_opencsg_capable && Preferences::inst()->getValue("advanced/enable_opencsg_opengl1x").toBool();
}
#endif

void QGLView::resizeGL(int w, int h)
{
#ifdef __APPLE__
  OSViewRef view = qt_mac_nativeview_for(this);
  if (view) {
    // Since this isn't an Objective-C file, manually call the runtime functions:
    if (objc_msgSend((id)view, sel_registerName("respondsToSelector:"), sel_registerName("setWantsBestResolutionOpenGLSurface:"))) {
      objc_msgSend((id)view, sel_registerName("setWantsBestResolutionOpenGLSurface:"), 1);
    }
  
    OSWindowRef window = qt_mac_window_for(this);
    if (window && objc_msgSend((id)window, sel_registerName("respondsToSelector:"), sel_registerName("backingScaleFactor"))) {
#if __LP64__
      scale_factor = ((double (*)(id, SEL))(void *)objc_msgSend)((id)window, sel_registerName("backingScaleFactor"));
#else
      scale_factor = objc_msgSend_fpret((id)window, sel_registerName("backingScaleFactor"));
#endif
    }
  }
#endif

  GLView::resizeGL(w,h);
  GLView::setupGimbalCamPerspective();
}

void QGLView::paintGL()
{
	GLView::gimbalCamPaintGL();

  if (statusLabel) {
    QString msg;
    msg.sprintf("Viewport: translate = [ %.2f %.2f %.2f ], rotate = [ %.2f %.2f %.2f ], distance = %.2f",
      -cam.object_trans.x(), -cam.object_trans.y(), -cam.object_trans.z(),
      fmodf(360 - cam.object_rot.x() + 90, 360), fmodf(360 - cam.object_rot.y(), 360), fmodf(360 - cam.object_rot.z(), 360), cam.viewer_distance);
    statusLabel->setText(msg);
  }

  if (running_under_wine) swapBuffers();
}

void QGLView::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Plus) {
    cam.viewer_distance *= 0.9;
    updateGL();
    return;
  }
  if (event->key() == Qt::Key_Minus) {
    cam.viewer_distance /= 0.9;
    updateGL();
    return;
  }
}

void QGLView::wheelEvent(QWheelEvent *event)
{
  cam.viewer_distance *= pow(0.9, event->delta() / 120.0);
  updateGL();
}

void QGLView::mousePressEvent(QMouseEvent *event)
{
  mouse_drag_active = true;
  last_mouse = event->globalPos();
}

void QGLView::normalizeAngle(GLdouble& angle)
{
  while(angle < 0)
    angle += 360;
  while(angle > 360)
    angle -= 360;
}

void QGLView::mouseMoveEvent(QMouseEvent *event)
{
  QPoint this_mouse = event->globalPos();
  double dx = (this_mouse.x()-last_mouse.x()) * 0.7;
  double dy = (this_mouse.y()-last_mouse.y()) * 0.7;
  if (mouse_drag_active) {
    if (event->buttons() & Qt::LeftButton
#ifdef Q_WS_MAC
        && !(event->modifiers() & Qt::MetaModifier)
#endif
      ) {
      // Left button rotates in xz, Shift-left rotates in xy
      // On Mac, Ctrl-Left is handled as right button on other platforms
      cam.object_rot.x() += dy;
      if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0)
        cam.object_rot.y() += dx;
      else
        cam.object_rot.z() += dx;

      normalizeAngle(cam.object_rot.x());
      normalizeAngle(cam.object_rot.y());
      normalizeAngle(cam.object_rot.z());
    } else {
      // Right button pans in the xz plane
      // Middle button pans in the xy plane
      // Shift-right and Shift-middle zooms
      if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
        cam.viewer_distance += (GLdouble)dy;
      } else {

      double mx = +(dx) * cam.viewer_distance/1000;
      double mz = -(dy) * cam.viewer_distance/1000;

      double my = 0;
#if (QT_VERSION < QT_VERSION_CHECK(4, 7, 0))
      if (event->buttons() & Qt::MidButton) {
#else
      if (event->buttons() & Qt::MiddleButton) {
#endif
        my = mz;
        mz = 0;
        // actually lock the x-position
        // (turns out to be easier to use than xy panning)
        mx = 0;
      }

      Matrix3d aax, aay, aaz, tm3;
      aax = Eigen::AngleAxisd(-(cam.object_rot.x()/180) * M_PI, Vector3d::UnitX());
      aay = Eigen::AngleAxisd(-(cam.object_rot.y()/180) * M_PI, Vector3d::UnitY());
      aaz = Eigen::AngleAxisd(-(cam.object_rot.z()/180) * M_PI, Vector3d::UnitZ());
      tm3 = Matrix3d::Identity();
      tm3 = aaz * (aay * (aax * tm3));

      Matrix4d tm;
      tm = Matrix4d::Identity();
      for (int i=0;i<3;i++) for (int j=0;j<3;j++) tm(j,i)=tm3(j,i);

      Matrix4d vec;
      vec <<
        0,  0,  0,  mx,
        0,  0,  0,  my,
        0,  0,  0,  mz,
        0,  0,  0,  1
      ;
      tm = tm * vec;
      cam.object_trans.x() += tm(0,3);
      cam.object_trans.y() += tm(1,3);
      cam.object_trans.z() += tm(2,3);
      }
    }
    updateGL();
    emit doAnimateUpdate();
  }
  last_mouse = this_mouse;
}

void QGLView::mouseReleaseEvent(QMouseEvent*)
{
  mouse_drag_active = false;
  releaseMouse();
}

static void convertFromGLImage(QImage &img, int w, int h, bool alpha_format, bool include_alpha)
{
  assert (QSysInfo::ByteOrder != QSysInfo::BigEndian);
  // OpenGL gives ABGR (i.e. RGBA backwards); Qt wants ARGB
  for (int y = 0; y < h; y++) {
    uint *q = (uint*)img.scanLine(y);
    for (int x=0; x < w; ++x) {
      const uint pixel = *q;
      if (alpha_format && include_alpha) {
        *q = ((pixel << 16) & 0xff0000) | ((pixel >> 16) & 0xff)
             | (pixel & 0xff00ff00);
      } else {
        *q = 0xff000000 | ((pixel << 16) & 0xff0000)
             | ((pixel >> 16) & 0xff) | (pixel & 0x00ff00);
      }

      q++;
    }
  }
  img = img.mirrored();
}

QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha)
{
  QImage img(size, (alpha_format && include_alpha) ? QImage::Format_ARGB32_Premultiplied
                                                   : QImage::Format_RGB32);
  int w = size.width();
  int h = size.height();
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
  convertFromGLImage(img, w, h, alpha_format, include_alpha);
  return img;
}



bool QGLView::save(const char *filename)
{
  // grabFrameBuffer() doesn't work right with a scale factor != 1, so don't
  // use it.

  makeCurrent();
  QImage img;
  int w = width() * scale_factor;
  int h = height() * scale_factor;
  if (format().rgba()) {
    img = qt_gl_read_framebuffer(QSize(w, h), format().alpha(), /*withAlpha=*/false);
  }

  return img.save(filename, "PNG");
}


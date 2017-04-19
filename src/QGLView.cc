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

#include "qtgettext.h"
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

#include <stdio.h>
#include <sstream>

#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif

QGLView::QGLView(QWidget *parent) :
#ifdef USE_QOPENGLWIDGET
	QOpenGLWidget(parent)
#else
	QGLWidget(parent)
#endif
{
  init();
}

#if defined(_WIN32) && !defined(USE_QOPENGLWIDGET)
static bool running_under_wine = false;
#endif

void QGLView::init()
{
  resetView();

  this->mouse_drag_active = false;
  this->statusLabel = nullptr;

  setMouseTracking(true);



#if defined(_WIN32) && !defined(USE_QOPENGLWIDGET)
// see paintGL() + issue160 + wine FAQ
#include <windows.h>
  HMODULE hntdll = GetModuleHandle(L"ntdll.dll");
  if (hntdll)
    if ( (void *)GetProcAddress(hntdll, "wine_get_version") )
      running_under_wine = true;
#endif
}

void QGLView::resetView()
{
	cam.resetView();
}

void QGLView::viewAll()
{
	if (auto renderer = this->getRenderer()) {
		auto bbox = renderer->getBoundingBox();
		cam.object_trans = -bbox.center();
		cam.viewAll(renderer->getBoundingBox());
	}
}

void QGLView::initializeGL()
{
  auto err = glewInit();
  if (err != GLEW_OK) {
    fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
  }
  GLView::initializeGL();
}

std::string QGLView::getRendererInfo() const
{
  std::stringstream info;
  info << glew_dump();
  // Don't translate as translated text in the Library Info dialog is not wanted
#ifdef USE_QOPENGLWIDGET
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
#else
  info << "\nQt graphics widget: QGLWidget";
#endif
  info << glew_extensions_dump();
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

  QString message;
  if (this->is_opencsg_capable) {
    message += _("Warning: You may experience OpenCSG rendering errors.\n\n");
  }
  else {
    message += _("Warning: Missing OpenGL capabilities for OpenCSG - OpenCSG has been disabled.\n\n");
    dialog->enableOpenCSGBox->hide();
  }
  message += _("It is highly recommended to use OpenSCAD on a system with "
							 "OpenGL 2.0 or later.\n"
							 "Your renderer information is as follows:\n");
  QString rendererinfo(_("GLEW version %1\n%2 (%3)\nOpenGL version %4\n"));
  message += rendererinfo.arg((const char *)glewGetString(GLEW_VERSION),
															(const char *)glGetString(GL_RENDERER),
															(const char *)glGetString(GL_VENDOR),
															(const char *)glGetString(GL_VERSION));

  dialog->setText(message);
  dialog->enableOpenCSGBox->setChecked(Preferences::inst()->getValue("advanced/enable_opencsg_opengl1x").toBool());
  dialog->exec();

  opencsg_support = this->is_opencsg_capable && Preferences::inst()->getValue("advanced/enable_opencsg_opengl1x").toBool();
}
#endif

void QGLView::resizeGL(int w, int h)
{
  GLView::resizeGL(w,h);
}

void QGLView::paintGL()
{
  GLView::paintGL();

  if (statusLabel) {
    Camera nc(cam);
    nc.gimbalDefaultTranslate();
		auto status = QString("%1 (%2x%3)")
			.arg(QString::fromStdString(nc.statusText()))
			.arg(size().rwidth())
			.arg(size().rheight());
    statusLabel->setText(status);
  }

#if defined(_WIN32) && !defined(USE_QOPENGLWIDGET)
  if (running_under_wine) swapBuffers();
#endif
}

void QGLView::mousePressEvent(QMouseEvent *event)
{
  mouse_drag_active = true;
  last_mouse = event->globalPos();
}

void QGLView::mouseDoubleClickEvent (QMouseEvent *event) {

	setupCamera();

	int viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];

	glGetIntegerv( GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	double x = event->pos().x() * this->getDPI();
	double y = viewport[3] - event->pos().y() * this->getDPI();
	GLfloat z = 0;

	glGetError(); // clear error state so we don't pick up previous errors
	glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
	auto glError = glGetError();
	if (glError != GL_NO_ERROR) {
		return;
	}

	if (z == 1) return; // outside object

	GLdouble px, py, pz;

	auto success = gluUnProject(x, y, z, modelview, projection, viewport, &px, &py, &pz);

	if (success == GL_TRUE) {
		cam.object_trans -= Vector3d(px, py, pz);
		updateGL();
		emit doAnimateUpdate();
	}
}

void QGLView::normalizeAngle(GLdouble& angle)
{
  while(angle < 0) angle += 360;
  while(angle > 360) angle -= 360;
}

void QGLView::mouseMoveEvent(QMouseEvent *event)
{
  auto this_mouse = event->globalPos();
  double dx = (this_mouse.x() - last_mouse.x()) * 0.7;
  double dy = (this_mouse.y() - last_mouse.y()) * 0.7;
  if (mouse_drag_active) {
    if (event->buttons() & Qt::LeftButton
#ifdef Q_OS_MAC
        && !(event->modifiers() & Qt::MetaModifier)
#endif
      ) {
      // Left button rotates in xz, Shift-left rotates in xy
      // On Mac, Ctrl-Left is handled as right button on other platforms
      cam.object_rot.x() += dy;
      if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
        cam.object_rot.y() += dx;
			}
      else {
        cam.object_rot.z() += dx;
			}

      normalizeAngle(cam.object_rot.x());
      normalizeAngle(cam.object_rot.y());
      normalizeAngle(cam.object_rot.z());
    } else {
      // Right button pans in the xz plane
      // Middle button pans in the xy plane
      // Shift-right and Shift-middle zooms
      if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
	      cam.zoom(-12.0 * dy);
      } else {

      double mx = +(dx) * 3.0 * cam.zoomValue() / QWidget::width();
      double mz = -(dy) * 3.0 * cam.zoomValue() / QWidget::height();

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

      Matrix3d aax, aay, aaz;
      aax = Eigen::AngleAxisd(-(cam.object_rot.x()/180) * M_PI, Vector3d::UnitX());
      aay = Eigen::AngleAxisd(-(cam.object_rot.y()/180) * M_PI, Vector3d::UnitY());
      aaz = Eigen::AngleAxisd(-(cam.object_rot.z()/180) * M_PI, Vector3d::UnitZ());
      Matrix3d tm3 = Matrix3d::Identity();
      tm3 = aaz * (aay * (aax * tm3));

      Matrix4d tm = Matrix4d::Identity();
      for (int i=0;i<3;i++) for (int j=0;j<3;j++) tm(j,i) = tm3(j,i);

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

const QImage & QGLView::grabFrame()
{
	// Force reading from front buffer. Some configurations will read from the back buffer here.
	glReadBuffer(GL_FRONT);
	this->frame = grabFrameBuffer();
	return this->frame;
}

bool QGLView::save(const char *filename)
{
  return this->frame.save(filename, "PNG");
}

void QGLView::wheelEvent(QWheelEvent *event)
{
#if QT_VERSION >= 0x050000
	this->cam.zoom(event->angleDelta().y());
#else
	this->cam.zoom(event->delta());
#endif
  updateGL();
}

void QGLView::ZoomIn(void)
{
  this->cam.zoom(120);
  updateGL();
}

void QGLView::ZoomOut(void)
{
  this->cam.zoom(-120);
  updateGL();
}

void QGLView::setOrthoMode(bool enabled)
{
	if (enabled) this->cam.setProjection(Camera::ProjectionType::ORTHOGONAL);
	else this->cam.setProjection(Camera::ProjectionType::PERSPECTIVE);
}

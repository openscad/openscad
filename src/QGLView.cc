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
#include "degree_trig.h"

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
#include "OpenCSGWarningDialog.h"
#include "QSettingsCached.h"


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
		cam.autocenter = true;
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
  std::ostringstream info;
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
		auto status = QString("%1 (%2x%3)")
			.arg(QString::fromStdString(cam.statusText()))
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
  if (!mouse_drag_active) {
    mouse_drag_moved = false;
  }

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

	const double dpi = this->getDPI();
	const double x = event->pos().x() * dpi;
	const double y = viewport[3] - event->pos().y() * dpi;
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
    mouse_drag_moved = true;
		if (event->buttons() & Qt::LeftButton
#ifdef Q_OS_MAC
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

  if (!mouse_drag_moved
      && (event->button() == Qt::RightButton)) {
    QPoint point = event->pos();
    //point.setY(this->height() - point.y());
    emit doSelectObject(point);
  }
  mouse_drag_moved = false;
}

const QImage & QGLView::grabFrame()
{
	// Force reading from front buffer. Some configurations will read from the back buffer here.
	glReadBuffer(GL_FRONT);
	this->frame = grabFrameBuffer();
	return this->frame;
}

bool QGLView::save(const char *filename) const
{
  return this->frame.save(filename, "PNG");
}

void QGLView::wheelEvent(QWheelEvent *event)
{
	const auto pos = event->pos();
	const int v = event->angleDelta().y();
	if (this->mouseCentricZoom) {
		zoomCursor(pos.x(), pos.y(), v);
	} else {
		zoom(v, true);
	}
}

void QGLView::ZoomIn(void)
{
    zoom(120, true);
}

void QGLView::ZoomOut(void)
{
    zoom(-120, true);
}

void QGLView::zoom(double v, bool relative)
{
    this->cam.zoom(v, relative);
    updateGL();
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
  const auto mx = ratio*screen_x*(aspectratio*height);
  const auto mz = ratio*screen_y*height;
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
    updateGL();
    emit doAnimateUpdate();
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
    updateGL();
    emit doAnimateUpdate();
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

    updateGL();
    emit doAnimateUpdate();
}

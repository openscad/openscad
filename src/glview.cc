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

#include "GLView.h"
#include "Preferences.h"
#include "renderer.h"
#include "rendersettings.h"
#include "linalg.h"

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

#ifdef _WIN32
#include <GL/wglew.h>
#elif !defined(__APPLE__)
#include <GL/glxew.h>
#endif

#define FAR_FAR_AWAY 100000.0

GLView::GLView(QWidget *parent) : QGLWidget(parent), renderer(NULL)
{
	init();
}

GLView::GLView(const QGLFormat & format, QWidget *parent) : QGLWidget(format, parent)
{
	init();
}

static bool running_under_wine = false;

void GLView::init()
{
	this->viewer_distance = 500;
	this->object_rot_x = 35;
	this->object_rot_y = 0;
	this->object_rot_z = -25;
	this->object_trans_x = 0;
	this->object_trans_y = 0;
	this->object_trans_z = 0;

	this->mouse_drag_active = false;

	this->showedges = false;
	this->showfaces = true;
	this->orthomode = false;
	this->showaxes = false;
	this->showcrosshairs = false;

	for (int i = 0; i < 10; i++)
		this->shaderinfo[i] = 0;

	this->statusLabel = NULL;

	setMouseTracking(true);
#ifdef ENABLE_OPENCSG
	this->is_opencsg_capable = false;
	this->has_shaders = false;
	this->opencsg_support = true;
	static int sId = 0;
	this->opencsg_id = sId++;
#endif

// see paintGL() + issue160 + wine FAQ
#ifdef _WIN32
#include <windows.h>
	HMODULE hntdll = GetModuleHandle(L"ntdll.dll");
	if (hntdll)
		if ( (void *)GetProcAddress(hntdll, "wine_get_version") )
			running_under_wine = true;
#endif
}

void GLView::setRenderer(Renderer *r)
{
	this->renderer = r;
	if (r) updateGL(); // Let the last image stay, e.g. to avoid animation flickering
}

void GLView::initializeGL()
{
	glEnable(GL_DEPTH_TEST);
	glDepthRange(-FAR_FAR_AWAY, +FAR_FAR_AWAY);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat light_position0[] = {-1.0, -1.0, +1.0, 0.0};
	GLfloat light_position1[] = {+1.0, +1.0, -1.0, 0.0};

	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

#ifdef ENABLE_OPENCSG
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
	}

	GLint rbits, gbits, bbits, abits, dbits, sbits;
	glGetIntegerv(GL_RED_BITS, &rbits);
	glGetIntegerv(GL_GREEN_BITS, &gbits);
	glGetIntegerv(GL_BLUE_BITS, &bbits);
	glGetIntegerv(GL_ALPHA_BITS, &abits);
	glGetIntegerv(GL_DEPTH_BITS, &dbits);
	glGetIntegerv(GL_STENCIL_BITS, &sbits);


	this->rendererInfo.sprintf("GLEW version %s\n"
														 "OpenGL version %s\n"
														 "%s (%s)\n\n"
														 "RGBA(%d%d%d%d), depth(%d), stencil(%d)\n"
														 "Extensions:\n"
														 "%s\n",
														 glewGetString(GLEW_VERSION),
														 glGetString(GL_VERSION),
														 glGetString(GL_RENDERER),
														 glGetString(GL_VENDOR),
														 rbits, gbits, bbits, abits, dbits, sbits,
														 glGetString(GL_EXTENSIONS));
// FIXME: glGetString(GL_EXTENSIONS) is deprecated in OpenGL 3.0.
// Use: glGetIntegerv(GL_NUM_EXTENSIONS, &NumberOfExtensions) and 
// glGetStringi(GL_EXTENSIONS, i)

	const char *openscad_disable_gl20_env = getenv("OPENSCAD_DISABLE_GL20");
	if (openscad_disable_gl20_env && !strcmp(openscad_disable_gl20_env, "0")) {
		openscad_disable_gl20_env = NULL;
	}

	// All OpenGL 2 contexts are OpenCSG capable
	if (GLEW_VERSION_2_0) {
		if (!openscad_disable_gl20_env) {
			this->is_opencsg_capable = true;
			this->has_shaders = true;
		}
	}
	// If OpenGL < 2, check for extensions
	else {
		if (GLEW_ARB_framebuffer_object) this->is_opencsg_capable = true;
		else if (GLEW_EXT_framebuffer_object && GLEW_EXT_packed_depth_stencil) {
			this->is_opencsg_capable = true;
		}
#ifdef WIN32
		else if (WGLEW_ARB_pbuffer && WGLEW_ARB_pixel_format) this->is_opencsg_capable = true;
#elif !defined(__APPLE__)
		else if (GLXEW_SGIX_pbuffer && GLXEW_SGIX_fbconfig) this->is_opencsg_capable = true;
#endif
	}

	if (!GLEW_VERSION_2_0 || !this->is_opencsg_capable) {
		if (Preferences::inst()->getValue("advanced/opencsg_show_warning").toBool()) {
			QTimer::singleShot(0, this, SLOT(display_opencsg_warning()));
		}
	}
	if (opencsg_support && this->has_shaders) {
  /*
		Uniforms:
		  1 color1 - face color
			2 color2 - edge color
			7 xscale
			8 yscale

		Attributes:
		  3 trig
			4 pos_b
			5 pos_c
			6 mask

		Other:
		  9 width
			10 height

		Outputs:
		  tp
			tr
			shading
	 */
		const char *vs_source =
			"uniform float xscale, yscale;\n"
			"attribute vec3 pos_b, pos_c;\n"
			"attribute vec3 trig, mask;\n"
			"varying vec3 tp, tr;\n"
			"varying float shading;\n"
			"void main() {\n"
			"  vec4 p0 = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
			"  vec4 p1 = gl_ModelViewProjectionMatrix * vec4(pos_b, 1.0);\n"
			"  vec4 p2 = gl_ModelViewProjectionMatrix * vec4(pos_c, 1.0);\n"
			"  float a = distance(vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));\n"
			"  float b = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p1.x/p1.w, yscale*p1.y/p1.w));\n"
			"  float c = distance(vec2(xscale*p0.x/p0.w, yscale*p0.y/p0.w), vec2(xscale*p2.x/p2.w, yscale*p2.y/p2.w));\n"
			"  float s = (a + b + c) / 2.0;\n"
			"  float A = sqrt(s*(s-a)*(s-b)*(s-c));\n"
			"  float ha = 2.0*A/a;\n"
			"  gl_Position = p0;\n"
			"  tp = mask * ha;\n"
			"  tr = trig;\n"
			"  vec3 normal, lightDir;\n"
			"  normal = normalize(gl_NormalMatrix * gl_Normal);\n"
			"  lightDir = normalize(vec3(gl_LightSource[0].position));\n"
			"  shading = abs(dot(normal, lightDir));\n"
			"}\n";

		/*
			Inputs:
			  tp && tr - if any components of tp < tr, use color2 (edge color)
				shading  - multiplied by color1. color2 is is without lighting
		 */
		const char *fs_source =
			"uniform vec4 color1, color2;\n"
			"varying vec3 tp, tr, tmp;\n"
			"varying float shading;\n"
			"void main() {\n"
			"  gl_FragColor = vec4(color1.r * shading, color1.g * shading, color1.b * shading, color1.a);\n"
			"  if (tp.x < tr.x || tp.y < tr.y || tp.z < tr.z)\n"
			"    gl_FragColor = color2;\n"
			"}\n";

		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, (const GLchar**)&vs_source, NULL);
		glCompileShader(vs);

		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, (const GLchar**)&fs_source, NULL);
		glCompileShader(fs);

		GLuint edgeshader_prog = glCreateProgram();
		glAttachShader(edgeshader_prog, vs);
		glAttachShader(edgeshader_prog, fs);
		glLinkProgram(edgeshader_prog);

		shaderinfo[0] = edgeshader_prog;
		shaderinfo[1] = glGetUniformLocation(edgeshader_prog, "color1");
		shaderinfo[2] = glGetUniformLocation(edgeshader_prog, "color2");
		shaderinfo[3] = glGetAttribLocation(edgeshader_prog, "trig");
		shaderinfo[4] = glGetAttribLocation(edgeshader_prog, "pos_b");
		shaderinfo[5] = glGetAttribLocation(edgeshader_prog, "pos_c");
		shaderinfo[6] = glGetAttribLocation(edgeshader_prog, "mask");
		shaderinfo[7] = glGetUniformLocation(edgeshader_prog, "xscale");
		shaderinfo[8] = glGetUniformLocation(edgeshader_prog, "yscale");

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			fprintf(stderr, "OpenGL Error: %s\n", gluErrorString(err));
		}

		GLint status;
		glGetProgramiv(edgeshader_prog, GL_LINK_STATUS, &status);
		if (status == GL_FALSE) {
			int loglen;
			char logbuffer[1000];
			glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
			fprintf(stderr, "OpenGL Program Linker Error:\n%.*s", loglen, logbuffer);
		} else {
			int loglen;
			char logbuffer[1000];
			glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
			if (loglen > 0) {
				fprintf(stderr, "OpenGL Program Link OK:\n%.*s", loglen, logbuffer);
			}
			glValidateProgram(edgeshader_prog);
			glGetProgramInfoLog(edgeshader_prog, sizeof(logbuffer), &loglen, logbuffer);
			if (loglen > 0) {
				fprintf(stderr, "OpenGL Program Validation results:\n%.*s", loglen, logbuffer);
			}
		}
	}
#endif /* ENABLE_OPENCSG */
}

#ifdef ENABLE_OPENCSG
void GLView::display_opencsg_warning()
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

void GLView::resizeGL(int w, int h)
{
#ifdef ENABLE_OPENCSG
	shaderinfo[9] = w;
	shaderinfo[10] = h;
#endif
	glViewport(0, 0, w, h);
	w_h_ratio = sqrt((double)w / (double)h);

	setupPerspective();
}

void GLView::setupPerspective()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-w_h_ratio, +w_h_ratio, -(1/w_h_ratio), +(1/w_h_ratio), +10.0, +FAR_FAR_AWAY);
	gluLookAt(0.0, -viewer_distance, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
}

void GLView::setupOrtho(double distance, bool offset)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(offset)
		glTranslated(-0.8, -0.8, 0);
	double l = distance/10;
	glOrtho(-w_h_ratio*l, +w_h_ratio*l,
			-(1/w_h_ratio)*l, +(1/w_h_ratio)*l,
			-FAR_FAR_AWAY, +FAR_FAR_AWAY);
	gluLookAt(0.0, -viewer_distance, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
}

void GLView::paintGL()
{
	glEnable(GL_LIGHTING);

	if (orthomode) setupOrtho(viewer_distance);
	else setupPerspective();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Color4f bgcol = RenderSettings::inst()->color(RenderSettings::BACKGROUND_COLOR);
	glClearColor(bgcol[0], bgcol[1], bgcol[2], 0.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glRotated(object_rot_x, 1.0, 0.0, 0.0);
	glRotated(object_rot_y, 0.0, 1.0, 0.0);
	glRotated(object_rot_z, 0.0, 0.0, 1.0);

  // FIXME: Crosshairs and axes are lighted, this doesn't make sense and causes them
  // to change color based on view orientation.
	if (showcrosshairs)
	{
		glLineWidth(3);
		Color4f col = RenderSettings::inst()->color(RenderSettings::CROSSHAIR_COLOR);
		glColor3f(col[0], col[1], col[2]);
		glBegin(GL_LINES);
		for (double xf = -1; xf <= +1; xf += 2)
		for (double yf = -1; yf <= +1; yf += 2) {
			double vd = viewer_distance/20;
			glVertex3d(-xf*vd, -yf*vd, -vd);
			glVertex3d(+xf*vd, +yf*vd, +vd);
		}
		glEnd();
	}

	glTranslated(object_trans_x, object_trans_y, object_trans_z);

	// Large gray axis cross inline with the model
  // FIXME: This is always gray - adjust color to keep contrast with background
	if (showaxes)
	{
		glLineWidth(1);
		glColor3d(0.5, 0.5, 0.5);
		glBegin(GL_LINES);
		double l = viewer_distance/10;
		glVertex3d(-l, 0, 0);
		glVertex3d(+l, 0, 0);
		glVertex3d(0, -l, 0);
		glVertex3d(0, +l, 0);
		glVertex3d(0, 0, -l);
		glVertex3d(0, 0, +l);
		glEnd();
	}

	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

	glLineWidth(2);
	glColor3d(1.0, 0.0, 0.0);

	if (this->renderer) {
#if defined(ENABLE_MDI) && defined(ENABLE_OPENCSG)
		// FIXME: This belongs in the OpenCSG renderer, but it doesn't know about this ID yet
		OpenCSG::setContext(this->opencsg_id);
#endif
		this->renderer->draw(showfaces, showedges);
	}

	// Small axis cross in the lower left corner
	if (showaxes)
	{
		glDepthFunc(GL_ALWAYS);

		setupOrtho(1000,true);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotated(object_rot_x, 1.0, 0.0, 0.0);
		glRotated(object_rot_y, 0.0, 1.0, 0.0);
		glRotated(object_rot_z, 0.0, 0.0, 1.0);

		glLineWidth(1);
		glBegin(GL_LINES);
		glColor3d(1.0, 0.0, 0.0);
		glVertex3d(0, 0, 0); glVertex3d(10, 0, 0);
		glColor3d(0.0, 1.0, 0.0);
		glVertex3d(0, 0, 0); glVertex3d(0, 10, 0);
		glColor3d(0.0, 0.0, 1.0);
		glVertex3d(0, 0, 0); glVertex3d(0, 0, 10);
		glEnd();

		GLdouble mat_model[16];
		glGetDoublev(GL_MODELVIEW_MATRIX, mat_model);

		GLdouble mat_proj[16];
		glGetDoublev(GL_PROJECTION_MATRIX, mat_proj);

		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		GLdouble xlabel_x, xlabel_y, xlabel_z;
		gluProject(12, 0, 0, mat_model, mat_proj, viewport, &xlabel_x, &xlabel_y, &xlabel_z);
		xlabel_x = round(xlabel_x); xlabel_y = round(xlabel_y);

		GLdouble ylabel_x, ylabel_y, ylabel_z;
		gluProject(0, 12, 0, mat_model, mat_proj, viewport, &ylabel_x, &ylabel_y, &ylabel_z);
		ylabel_x = round(ylabel_x); ylabel_y = round(ylabel_y);

		GLdouble zlabel_x, zlabel_y, zlabel_z;
		gluProject(0, 0, 12, mat_model, mat_proj, viewport, &zlabel_x, &zlabel_y, &zlabel_z);
		zlabel_x = round(zlabel_x); zlabel_y = round(zlabel_y);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glTranslated(-1, -1, 0);
		glScaled(2.0/viewport[2], 2.0/viewport[3], 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// FIXME: This was an attempt to keep contrast with background, but is suboptimal
		// (e.g. nearly invisible against a gray background).
//		int r,g,b;
//		r=g=b=0;
//		bgcol.getRgb(&r, &g, &b);
//		glColor3f((255.0f-r)/255.0f, (255.0f-g)/255.0f, (255.0f-b)/255.0f);
		glColor3f(0.0f, 0.0f, 0.0f);
		glBegin(GL_LINES);
		// X Label
		glVertex3d(xlabel_x-3, xlabel_y-3, 0); glVertex3d(xlabel_x+3, xlabel_y+3, 0);
		glVertex3d(xlabel_x-3, xlabel_y+3, 0); glVertex3d(xlabel_x+3, xlabel_y-3, 0);
		// Y Label
		glVertex3d(ylabel_x-3, ylabel_y-3, 0); glVertex3d(ylabel_x+3, ylabel_y+3, 0);
		glVertex3d(ylabel_x-3, ylabel_y+3, 0); glVertex3d(ylabel_x, ylabel_y, 0);
		// Z Label
		glVertex3d(zlabel_x-3, zlabel_y-3, 0); glVertex3d(zlabel_x+3, zlabel_y-3, 0);
		glVertex3d(zlabel_x-3, zlabel_y+3, 0); glVertex3d(zlabel_x+3, zlabel_y+3, 0);
		glVertex3d(zlabel_x-3, zlabel_y-3, 0); glVertex3d(zlabel_x+3, zlabel_y+3, 0);
		glEnd();

		//Restore perspective for next paint
		if(!orthomode)
			setupPerspective();
	}

	if (statusLabel) {
		QString msg;
		msg.sprintf("Viewport: translate = [ %.2f %.2f %.2f ], rotate = [ %.2f %.2f %.2f ], distance = %.2f",
			-object_trans_x, -object_trans_y, -object_trans_z,
			fmodf(360 - object_rot_x + 90, 360), fmodf(360 - object_rot_y, 360), fmodf(360 - object_rot_z, 360), viewer_distance);
		statusLabel->setText(msg);
	}

	if (running_under_wine) swapBuffers();
}

void GLView::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Plus) {
		viewer_distance *= 0.9;
		updateGL();
		return;
	}
	if (event->key() == Qt::Key_Minus) {
		viewer_distance /= 0.9;
		updateGL();
		return;
	}
}

void GLView::wheelEvent(QWheelEvent *event)
{
	viewer_distance *= pow(0.9, event->delta() / 120.0);
	updateGL();
}

void GLView::mousePressEvent(QMouseEvent *event)
{
	mouse_drag_active = true;
	last_mouse = event->globalPos();
}

void GLView::normalizeAngle(GLdouble& angle)
{
	while(angle < 0)
		angle += 360;
	while(angle > 360)
		angle -= 360;
}

void GLView::mouseMoveEvent(QMouseEvent *event)
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
			object_rot_x += dy;
			if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0)
				object_rot_y += dx;
			else
				object_rot_z += dx;

			normalizeAngle(object_rot_x);
			normalizeAngle(object_rot_y);
			normalizeAngle(object_rot_z);
		} else {
			// Right button pans in the xz plane
			// Middle button pans in the xy plane
			// Shift-right and Shift-middle zooms
			if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
				viewer_distance += (GLdouble)dy;
			} else {

      double mx = +(dx) * viewer_distance/1000;
      double mz = -(dy) * viewer_distance/1000;

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
			aax = Eigen::AngleAxisd(-(object_rot_x/180) * M_PI, Vector3d::UnitX());
			aay = Eigen::AngleAxisd(-(object_rot_y/180) * M_PI, Vector3d::UnitY());
			aaz = Eigen::AngleAxisd(-(object_rot_z/180) * M_PI, Vector3d::UnitZ());
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
      object_trans_x += tm(0,3);
      object_trans_y += tm(1,3);
      object_trans_z += tm(2,3);
			}
		}
		updateGL();
		emit doAnimateUpdate();
	}
	last_mouse = this_mouse;
}

void GLView::mouseReleaseEvent(QMouseEvent*)
{
	mouse_drag_active = false;
	releaseMouse();
}

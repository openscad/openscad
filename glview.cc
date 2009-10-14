/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include "openscad.h"

#include <QApplication>
#include <QWheelEvent>
#include <QMouseEvent>

#define FAR_FAR_AWAY 100000.0

GLView::GLView(QWidget *parent) : QGLWidget(parent)
{
	viewer_distance = 500;
	object_rot_x = 35;
	object_rot_y = 0;
	object_rot_z = 25;
	object_trans_x = 0;
	object_trans_y = 0;
	object_trans_z = 0;

	mouse_drag_active = false;
	last_mouse_x = 0;
	last_mouse_y = 0;

	orthomode = false;
	showaxis = false;
	showcrosshairs = false;

	renderfunc = NULL;
	renderfunc_vp = NULL;

	for (int i = 0; i < 10; i++)
		shaderinfo[i] = 0;

	setMouseTracking(true);
}

extern GLint e1, e2, e3;

void GLView::initializeGL()
{
	glEnable(GL_DEPTH_TEST);
	glDepthRange(-FAR_FAR_AWAY, +FAR_FAR_AWAY);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(1.0, 1.0, 0.9, 0.0);

#ifdef ENABLE_OPENCSG
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
	}
	if (glewIsSupported("GL_VERSION_2_0"))
	{
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
	} else {
		fprintf(stdout, "GLEW: GL_VERSION_2_0 is not supported!\n");
	}
#endif /* ENABLE_OPENCSG */
}

void GLView::resizeGL(int w, int h)
{
#ifdef ENABLE_OPENCSG
	shaderinfo[9] = w;
	shaderinfo[10] = h;
#endif
	glViewport(0, 0, w, h);
	w_h_ratio = sqrt((double)w / (double)h);
}

void GLView::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (orthomode)
		glOrtho(-w_h_ratio*viewer_distance/10, +w_h_ratio*viewer_distance/10,
				-(1/w_h_ratio)*viewer_distance/10, +(1/w_h_ratio)*viewer_distance/10,
				-FAR_FAR_AWAY, +FAR_FAR_AWAY);
	else
		glFrustum(-w_h_ratio, +w_h_ratio, -(1/w_h_ratio), +(1/w_h_ratio), +10.0, +FAR_FAR_AWAY);
	gluLookAt(0.0, -viewer_distance, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

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

	glRotated(object_rot_x, 1.0, 0.0, 0.0);
	glRotated(object_rot_y, 0.0, 1.0, 0.0);
	glRotated(object_rot_z, 0.0, 0.0, 1.0);

	if (showcrosshairs)
	{
		glLineWidth(3);
		glColor3d(0.5, 0.0, 0.0);
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

	if (showaxis)
	{
		glLineWidth(1);
		glColor3d(0.5, 0.5, 0.5);
		glBegin(GL_LINES);
		glVertex3d(-viewer_distance/10, 0, 0);
		glVertex3d(+viewer_distance/10, 0, 0);
		glVertex3d(0, -viewer_distance/10, 0);
		glVertex3d(0, +viewer_distance/10, 0);
		glVertex3d(0, 0, -viewer_distance/10);
		glVertex3d(0, 0, +viewer_distance/10);
		glEnd();
	}

	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

	glLineWidth(2);
	glColor3d(1.0, 0.0, 0.0);

	if (renderfunc)
		renderfunc(renderfunc_vp);

	if (showaxis)
	{
		glDepthFunc(GL_ALWAYS);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glTranslated(-0.8, -0.8, 0);
		glOrtho(-w_h_ratio*1000/10, +w_h_ratio*1000/10,
				-(1/w_h_ratio)*1000/10, +(1/w_h_ratio)*1000/10,
				-FAR_FAR_AWAY, +FAR_FAR_AWAY);
		gluLookAt(0.0, -1000, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotated(object_rot_x, 1.0, 0.0, 0.0);
		glRotated(object_rot_y, 0.0, 1.0, 0.0);
		glRotated(object_rot_z, 0.0, 0.0, 1.0);

		glLineWidth(1);
		glColor3d(0.0, 0.0, 1.0);
		glBegin(GL_LINES);
		glVertex3d(0, 0, 0); glVertex3d(10, 0, 0);
		glVertex3d(0, 0, 0); glVertex3d(0, 10, 0);
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

		glColor3d(0.0, 0.0, 0.0);
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
	}
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
	last_mouse_x = event->globalX();
	last_mouse_y = event->globalY();
	grabMouse();
	setFocus();
	updateGL();
}

static void mat_id(double *trg)
{
	for (int i = 0; i < 16; i++)
		trg[i] = i%5 == 0;
}

static void mat_mul(double *trg, const double *m1, const double *m2)
{
	double m[16];
	for (int x = 0; x < 4; x++)
	for (int y = 0; y < 4; y++)
	{
		m[x+y*4] = 0;
		for (int i = 0; i < 4; i++)
			m[x+y*4] += m1[i+y*4] * m2[x+i*4];
	}
	for (int i = 0; i < 16; i++)
		trg[i] = m[i];
}

static void mat_rot(double *trg, double angle, double x, double y, double z)
{
	double s = sin(M_PI*angle/180), c = cos(M_PI*angle/180);
	double cc = 1 - c;
	double m[16] = {
		x*x*cc+c,	x*y*cc-z*s,	x*z*cc+y*s,	0,
		y*x*cc+z*s,	y*y*cc+c,	y*z*cc-x*s,	0,
		x*z*cc-y*s,	y*z*cc+x*s,	z*z*cc+c,	0,
		0,		0,		0,		1
	};
	for (int i = 0; i < 16; i++)
		trg[i] = m[i];
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
	int this_mouse_x = event->globalX();
	int this_mouse_y = event->globalY();
	if (mouse_drag_active) {
		if ((event->buttons() & Qt::LeftButton) != 0) {
			object_rot_x += (this_mouse_y-last_mouse_y) * 0.7;
			if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0)
				object_rot_y += (this_mouse_x-last_mouse_x) * 0.7;
			else
				object_rot_z += (this_mouse_x-last_mouse_x) * 0.7;
		} else {
			double mx = +(this_mouse_x-last_mouse_x) * viewer_distance/1000;
			double my = -(this_mouse_y-last_mouse_y) * viewer_distance/1000;
			double rx[16], ry[16], rz[16], tm[16];
			mat_rot(rx, -object_rot_x, 1.0, 0.0, 0.0);
			mat_rot(ry, -object_rot_y, 0.0, 1.0, 0.0);
			mat_rot(rz, -object_rot_z, 0.0, 0.0, 1.0);
			mat_id(tm);
			mat_mul(tm, rx, tm);
			mat_mul(tm, ry, tm);
			mat_mul(tm, rz, tm);
			double vec[16] = {
				0,	0,	0,	mx,
				0,	0,	0,	0,
				0,	0,	0,	my,
				0,	0,	0,	1
			};
			if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
				vec[3] = 0;
				vec[7] = my;
				vec[11] = 0;
			}
			mat_mul(tm, tm, vec);
			object_trans_x += tm[3];
			object_trans_y += tm[7];
			object_trans_z += tm[11];
		}
		updateGL();
	}
	last_mouse_x = this_mouse_x;
	last_mouse_y = this_mouse_y;
}

void GLView::mouseReleaseEvent(QMouseEvent*)
{
	mouse_drag_active = false;
	releaseMouse();
	updateGL();
}


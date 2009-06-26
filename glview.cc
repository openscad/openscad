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

#include <QWheelEvent>
#include <QMouseEvent>
#include <GL/glew.h>

#define FAR_FAR_AWAY 100000.0

GLView::GLView(QWidget *parent) : QGLWidget(parent)
{
	viewer_distance = 20;
	object_rot_y = 35;
	object_rot_z = 25;

	mouse_drag_active = false;
	last_mouse_x = 0;
	last_mouse_y = 0;

	renderfunc = NULL;
	renderfunc_vp = NULL;
	edgeshader_prog = 0;

	setMouseTracking(true);
}

extern GLint e1, e2, e3;

void GLView::initializeGL()
{
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
	}

	glEnable(GL_DEPTH_TEST);
	glDepthRange(-FAR_FAR_AWAY, +FAR_FAR_AWAY);

	glClearColor(1.0, 1.0, 0.9, 0.0);

	if (glewIsSupported("GL_VERSION_2_0"))
	{
		char *vs_source =
			"attribute float e1, e2, e3;\n"
			"varying float ve1, ve2, ve3;\n"
			"void main() {\n"
			"       gl_FrontColor = gl_Color;\n"
			"       gl_Position = ftransform();\n"
			"       ve1 = e1; ve2 = e2; ve3 = e3;\n"
			"}\n";

		char *fs_source =
			"varying float ve1, ve2, ve3;\n"
			"void main() {\n"
			"       gl_FragColor = vec4(249.0/255.0, 215.0/255.0, 44.0/255.0, 1.0);\n"
			"       if (ve1 > 0.95 || ve2 > 0.95 || ve3 > 0.95)\n"
			"               gl_FragColor = vec4(255.0/255.0, 236.0/255.0, 94.0/255.0, 1.0);\n"
			"}\n";

		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, (const GLchar**)&vs_source, NULL);
		glCompileShader(vs);

		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, (const GLchar**)&fs_source, NULL);
		glCompileShader(fs);

		edgeshader_prog = glCreateProgram();
		glAttachShader(edgeshader_prog, vs);
		glAttachShader(edgeshader_prog, fs);
		glLinkProgram(edgeshader_prog);

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
}

void GLView::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	w_h_ratio = sqrt((double)w / (double)h);
}

void GLView::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-w_h_ratio, +w_h_ratio, -(1/w_h_ratio), +(1/w_h_ratio), +10.0, +FAR_FAR_AWAY);
	gluLookAt(0.0, -viewer_distance, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotated(object_rot_y, 1.0, 0.0, 0.0);
	glRotated(object_rot_z, 0.0, 0.0, 1.0);

	glDepthFunc(GL_LESS);
	glDisable(GL_LIGHTING);

#if 0
	glLineWidth(1);
	glColor3d(0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3d(0, 0, 0); glVertex3d(10, 0, 0);
	glVertex3d(0, 0, 0); glVertex3d(0, 10, 0);
	glVertex3d(0, 0, 0); glVertex3d(0, 0, 10);
	glEnd();
#endif

	glLineWidth(2);
	glColor3d(1.0, 0.0, 0.0);

	if (renderfunc)
		renderfunc(renderfunc_vp);
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
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
	int this_mouse_x = event->globalX();
	int this_mouse_y = event->globalY();
	if (mouse_drag_active) {
		object_rot_y += (this_mouse_y-last_mouse_y) * 0.7;
		object_rot_z += (this_mouse_x-last_mouse_x) * 0.7;
		updateGL();
	}
	last_mouse_x = this_mouse_x;
	last_mouse_y = this_mouse_y;
}

void GLView::mouseReleaseEvent(QMouseEvent*)
{
	mouse_drag_active = false;
	releaseMouse();
}


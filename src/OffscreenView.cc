#include <GL/glew.h>
#include "OffscreenView.h"
#include "system-gl.h"
#include "renderer.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <sstream>

#define FAR_FAR_AWAY 100000.0

OffscreenView::OffscreenView(size_t width, size_t height)
	: orthomode(false), showaxes(false), showfaces(true), showedges(false),
		object_rot(35, 0, 25), camera_eye(0, 0, 0), camera_center(0, 0, 0)
{
	for (int i = 0; i < 10; i++) this->shaderinfo[i] = 0;
	this->ctx = create_offscreen_context(width, height);
	if ( this->ctx == NULL ) throw -1;

	initializeGL();
	resizeGL(width, height);
}

OffscreenView::~OffscreenView()
{
	teardown_offscreen_context(this->ctx);
}

void OffscreenView::setRenderer(Renderer* r)
{
	this->renderer = r;
}

void OffscreenView::initializeGL()
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


}

void OffscreenView::resizeGL(int w, int h)
{
	this->width = w;
	this->height = h;
	glViewport(0, 0, w, h);
	w_h_ratio = sqrt((double)w / (double)h);
}

void OffscreenView::setupPerspective()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double dist = (this->camera_center - this->camera_eye).norm();
	gluPerspective(45, w_h_ratio, 0.1*dist, 100*dist);
}

void OffscreenView::setupOrtho(bool offset)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (offset) glTranslated(-0.8, -0.8, 0);
	double l = (this->camera_center - this->camera_eye).norm() / 10;
	glOrtho(-w_h_ratio*l, +w_h_ratio*l,
					-(1/w_h_ratio)*l, +(1/w_h_ratio)*l,
					-FAR_FAR_AWAY, +FAR_FAR_AWAY);
}

void OffscreenView::paintGL()
{
	glEnable(GL_LIGHTING);

	if (orthomode) setupOrtho();
	else setupPerspective();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(1.0f, 1.0f, 0.92f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	gluLookAt(this->camera_eye[0], this->camera_eye[1], this->camera_eye[2],
						this->camera_center[0], this->camera_center[1], this->camera_center[2], 
						0.0, 0.0, 1.0);

	// glRotated(object_rot[0], 1.0, 0.0, 0.0);
	// glRotated(object_rot[1], 0.0, 1.0, 0.0);
	// glRotated(object_rot[2], 0.0, 0.0, 1.0);

	// Large gray axis cross inline with the model
  // FIXME: This is always gray - adjust color to keep contrast with background
	if (showaxes)
	{
		glLineWidth(1);
		glColor3d(0.5, 0.5, 0.5);
		glBegin(GL_LINES);
		double l = 3*(this->camera_center - this->camera_eye).norm();
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
		this->renderer->draw(showfaces, showedges);
	}
}

bool OffscreenView::save(const char *filename)
{
	return save_framebuffer(this->ctx, filename);
}

bool OffscreenView::save(std::ostream &output)
{
	return save_framebuffer(this->ctx, output);
}

std::string OffscreenView::getInfo()
{
	std::stringstream out;
	GLint rbits, gbits, bbits, abits, dbits, sbits;
	glGetIntegerv(GL_RED_BITS, &rbits);
	glGetIntegerv(GL_GREEN_BITS, &gbits);
	glGetIntegerv(GL_BLUE_BITS, &bbits);
	glGetIntegerv(GL_ALPHA_BITS, &abits);
	glGetIntegerv(GL_DEPTH_BITS, &dbits);
	glGetIntegerv(GL_STENCIL_BITS, &sbits);

	out << glew_dump(false)
	    << "FBO: RGBA(" << rbits << gbits << bbits << abits
	    << "), depth(" << dbits
	    << "), stencil(" << sbits << ")\n"
	    << offscreen_context_getinfo(this->ctx);

	return out.str();
}

void OffscreenView::setCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &center)
{
	this->camera_eye = pos;
	this->camera_center = center;
}


#include "GLView.h"

#include "printutils.h"
#include <iostream>

GLView::GLView()
{
	std::cout << "glview();" << std::endl;
	this->renderer = NULL;
}

void GLView::setRenderer(Renderer* r)
{
	std::cout << "setr " << r << "\n"	<< std::endl;
	this->renderer = r;
}

void GLView::setGimbalCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &rot, double distance)
{
	PRINT("set gimbal camera not implemented");
}

void GLView::setupGimbalPerspective()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-w_h_ratio, +w_h_ratio, -(1/w_h_ratio), +(1/w_h_ratio), +10.0, +FAR_FAR_AWAY);
  gluLookAt(0.0, -viewer_distance, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
}

void GLView::setupGimbalOrtho(double distance, bool offset)
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

void GLView::setCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &center)
{
  this->camera_eye = pos;
  this->camera_center = center;
}

void GLView::setupPerspective()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  double dist = (this->camera_center - this->camera_eye).norm();
  gluPerspective(45, w_h_ratio, 0.1*dist, 100*dist);
}

void GLView::setupOrtho(bool offset)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (offset) glTranslated(-0.8, -0.8, 0);
  double l = (this->camera_center - this->camera_eye).norm() / 10;
  glOrtho(-w_h_ratio*l, +w_h_ratio*l,
          -(1/w_h_ratio)*l, +(1/w_h_ratio)*l,
          -FAR_FAR_AWAY, +FAR_FAR_AWAY);
}

/*
	void initializeGL(); //
	void resizeGL(int w, int h); //


	void paintGL(); //
	bool save(const char *filename); //
	//bool save(std::ostream &output); // not implemented in qgl?

	GLint shaderinfo[11];  //

*/

#include "GLView.h"

#include "printutils.h"
#include <iostream>

GLView::GLView()
{
	viewer_distance = 500;
	object_trans << 0, 0, 0;
  camera_eye << 0, 0, 0;
  camera_center << 0, 0, 0;
  showedges = false;
  showfaces = true;
  orthomode = false;
  showaxes = false;
  showcrosshairs = false;
	renderer = NULL;
#ifdef ENABLE_OPENCSG
  is_opencsg_capable = false;
  has_shaders = false;
  opencsg_support = true;
  static int sId = 0;
  this->opencsg_id = sId++;
  for (int i = 0; i < 10; i++) this->shaderinfo[i] = 0;
#endif
}

void GLView::setRenderer(Renderer* r)
{
	renderer = r;
}

void GLView::resizeGL(int w, int h)
{
#ifdef ENABLE_OPENCSG
  shaderinfo[9] = w;
  shaderinfo[10] = h;
#endif
  this->width = w;
  this->height = h;
  glViewport(0, 0, w, h);
  w_h_ratio = sqrt((double)w / (double)h);
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


	void paintGL(); //
	bool save(const char *filename); //
	//bool save(std::ostream &output); // not implemented in qgl?

	GLint shaderinfo[11];  //

*/

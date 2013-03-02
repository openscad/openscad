#ifndef GLVIEW_H_
#define GLVIEW_H_

/* GLView: A basic OpenGL rectangle for rendering images.

This class is inherited by

*QGLview (for Qt GUI)
*OffscreenView (used in tests and for offscreen command-line rendering).

There are two different types of cameras

*Gimbal camera - uses Euler Angles, object translation, and viewer distance
*Vector camera - uses 'eye', 'center', and 'up' vectors

Currently, the two cameras are not kept in sync and they are not easily
interchangable in code QGLView uses GimbalCamera while OffscreenView can
use both (but defaults to Vector)

*/

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#ifndef _MSC_VER
#include <stdint.h>
#endif
#include "system-gl.h"
#include <iostream>
#include "renderer.h"
#include "Camera.h"

#define FAR_FAR_AWAY 100000.0

class GLView
{
public:
	GLView();
	void setRenderer(Renderer* r);
	Renderer *renderer;

	void initializeGL();
	void resizeGL(int w, int h);
	virtual void paintGL();

	void setGimbalCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &rot, double distance);
	void setupGimbalCamPerspective();
	void setupGimbalCamOrtho(double distance, bool offset=false);
	void gimbalCamPaintGL();

	void setVectorCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &center);
	void setupVectorCamPerspective();
	void setupVectorCamOrtho(bool offset=false);
	void vectorCamPaintGL();

	void setCamera( Camera &cam );

	void showCrosshairs();
	void showAxes();
	void showSmallaxes();

	virtual bool save(const char *filename) = 0;
	virtual std::string getRendererInfo() const = 0;

	size_t width;
	size_t height;
  double w_h_ratio;
  bool orthomode;
  bool showaxes;
  bool showfaces;
  bool showedges;
  bool showcrosshairs;

	Camera::CameraType camtype;
	VectorCamera vcam;
	GimbalCamera gcam;

#ifdef ENABLE_OPENCSG
  GLint shaderinfo[11];
  bool is_opencsg_capable;
  bool has_shaders;
  void enable_opencsg_shaders();
	virtual void display_opencsg_warning() = 0;
  bool opencsg_support;
  int opencsg_id;
#endif
};

#endif

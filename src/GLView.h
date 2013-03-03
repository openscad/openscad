#ifndef GLVIEW_H_
#define GLVIEW_H_

/* GLView: A basic OpenGL rectangle for rendering images.

This class is inherited by:

*QGLview - for Qt GUI
*OffscreenView - for offscreen rendering, in tests and from command-line

The view assumes either a Gimbal Camera (rotation,translation,distance)
or Vector Camera (eye,center/target) is being used. See Camera.h. The
cameras are not kept in sync.

QGLView only uses GimbalCamera while OffscreenView can use either one.
Some actions (showCrossHairs) only work properly on Gimbal Camera.

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

	void setCamera( Camera &cam );

	void setupGimbalCamPerspective();
	void setupGimbalCamOrtho(double distance, bool offset=false);
	void gimbalCamPaintGL();

	void setupVectorCamPerspective();
	void setupVectorCamOrtho(bool offset=false);
	void vectorCamPaintGL();

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

	Camera cam;

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

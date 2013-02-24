#ifndef GLVIEW_H_
#define GLVIEW_H_

/* GLView: A basic OpenGL rectangle for rendering images.

Inherited by QGLview (for QT GUI) and OffscreenView.

There are two different types of cameras. A 'gimbal' based camera set
using position & euler-angles (object_trans/object_rot/distance) and a
'plain' camera set using eye-position, 'look at' center point, and 'up'

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

#define FAR_FAR_AWAY 100000.0

class GLView
{
public:
	GLView();
	void setRenderer(Renderer* r);
	Renderer *renderer;
/*
	void initializeGL(); //

*/
	void resizeGL(int w, int h);

	void setGimbalCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &rot, double distance);
	void setupGimbalPerspective();
	void setupGimbalOrtho(double distance, bool offset=false);

	void setCamera(const Eigen::Vector3d &pos, const Eigen::Vector3d &center);
	void setupPerspective();
	void setupOrtho(bool offset=false);

	virtual bool save(const char *filename) = 0;
	virtual std::string getRendererInfo() const = 0;

	size_t width;
	size_t height;
	double viewer_distance;
  double w_h_ratio;
  bool orthomode;
  bool showaxes;
  bool showfaces;
  bool showedges;
  bool showcrosshairs;
	Eigen::Vector3d object_trans;
	Eigen::Vector3d object_rot;
  Eigen::Vector3d camera_eye;
  Eigen::Vector3d camera_center;

#ifdef ENABLE_OPENCSG
  GLint shaderinfo[11];
  bool is_opencsg_capable;
  bool has_shaders;
//  void enable_opencsg_shaders();
	virtual void display_opencsg_warning() = 0;
  bool opencsg_support;
  int opencsg_id;
#endif
/*
	void paintGL(); //
*/

};

#endif

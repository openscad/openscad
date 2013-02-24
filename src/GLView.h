#ifndef GLVIEW_H_
#define GLVIEW_H_

// A basic OpenGL viewing rectangle.
// Inherited by QGLview (for QT) and OffscreenView (non-QT)

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
	double viewer_distance;//
  double w_h_ratio;//
  bool orthomode;//
  bool showaxes;//
  bool showfaces;//
  bool showedges;//
	Eigen::Vector3d object_trans;
	Eigen::Vector3d object_rot;
  Eigen::Vector3d camera_eye;
  Eigen::Vector3d camera_center;

#ifdef ENABLE_OPENCSG
  GLint shaderinfo[11];
#endif
/*
	void paintGL(); //
*/

};

#endif

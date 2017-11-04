#pragma once

/* GLView: A basic OpenGL rectangle for rendering images.

   This class is inherited by:

 * QGLview - for Qt GUI
 * OffscreenView - for offscreen rendering, in tests and from command-line
   (This class is also overridden by NULLGL.cc for special experiments)

   The view assumes either a Gimbal Camera (rotation,translation,distance)
   or Vector Camera (eye,center/target) is being used. See Camera.h. The
   cameras are not kept in sync.

   QGLView only uses GimbalCamera while OffscreenView can use either one.
   Some actions (showCrossHairs) only work properly on Gimbal Camera.

 */

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include "system-gl.h"
#include <iostream>
#include "Camera.h"
#include "colormap.h"

class GLView
{
public:
	GLView();
	void setRenderer(class Renderer *r);
	Renderer *getRenderer() const { return this->renderer; }

	void initializeGL();
	void resizeGL(int w, int h);
	virtual void paintGL();

	void setCamera(const Camera &cam);
	void setupCamera();

	void setColorScheme(const ColorScheme &cs);
	void setColorScheme(const std::string &cs);
	void updateColorScheme();

	virtual bool save(const char *filename) = 0;
	virtual std::string getRendererInfo() const = 0;
	virtual float getDPI() { return 1.0f; }

	Renderer *renderer;
	const ColorScheme *colorscheme;
	Camera cam;
	double far_far_away;
	size_t width;
	size_t height;
	double aspectratio;
	bool orthomode;
	bool showaxes;
	bool showfaces;
	bool showedges;
	bool showcrosshairs;
	bool showscale;

#ifdef ENABLE_OPENCSG
	GLint shaderinfo[11];
	bool is_opencsg_capable;
	bool has_shaders;
	void enable_opencsg_shaders();
	virtual void display_opencsg_warning() = 0;
	bool opencsg_support;
	int opencsg_id;
#endif
private:
	void showCrosshairs();
	void showAxes(const Color4f &col);
	void showSmallaxes(const Color4f &col);
	void showScalemarkers(const Color4f &col);
	void decodeMarkerValue(double i, double l, int size_div_sm);
};

#ifndef OFFSCREENVIEW_H_
#define OFFSCREENVIEW_H_

#include "OffscreenContext.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#ifndef _MSC_VER
#include <stdint.h>
#endif
#include "system-gl.h"
#include <iostream>
#include "GLView.h"

class OffscreenView : public GLView
{
public:
	OffscreenView(size_t width, size_t height); // not
	~OffscreenView(); // not

	void initializeGL(); //
	void resizeGL(int w, int h); //

	void paintGL(); //
	bool save(const char *filename); //
	bool save(std::ostream &output); // not implemented in qgl?
  std::string getRendererInfo() const;

	GLint shaderinfo[11];  //

	OffscreenContext *ctx; // not
	size_t width;  // not
	size_t height; // not
};

#endif

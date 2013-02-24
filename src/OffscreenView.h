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
	void enable_opencsg_shaders();
	void initializeGL(); //
	void paintGL(); //
	bool save(std::ostream &output);
	OffscreenContext *ctx; // not

	// overrides
	bool save(const char *filename);
  std::string getRendererInfo() const;
};

#endif

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
	void paintGL(); //

	bool save(const char *filename);
	bool save(std::ostream &output);
  std::string getRendererInfo() const;
	OffscreenContext *ctx; // not

#ifdef ENABLE_OPENCSG
	bool is_opencsg_capable;
	bool has_shaders;
	void enable_opencsg_shaders();
  bool opencsg_support;
  int opencsg_id;
#endif
};

#endif

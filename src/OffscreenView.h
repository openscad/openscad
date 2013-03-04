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
	OffscreenView(size_t width, size_t height);
	~OffscreenView();
	bool save(std::ostream &output);
	OffscreenContext *ctx;

	// overrides
	bool save(const char *filename);
	std::string getRendererInfo() const;
#ifdef ENABLE_OPENCSG
	void display_opencsg_warning();
#endif
};

#endif

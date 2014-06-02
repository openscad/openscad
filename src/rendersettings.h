#pragma once

#include <map>
#include "linalg.h"
#include "colormap.h"

class RenderSettings
{
public:
	static RenderSettings *inst(bool erase = false);

	unsigned int openCSGTermLimit, img_width, img_height;
	double far_gl_clip_limit;
private:
	RenderSettings();
	~RenderSettings() {}
};

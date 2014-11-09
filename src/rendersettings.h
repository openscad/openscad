#pragma once

#include <map>
#include "linalg.h"

class RenderSettings
{
public:
	static RenderSettings *inst(bool erase = false);

	unsigned int openCSGTermLimit, img_width, img_height;
	double far_gl_clip_limit;
	std::string colorscheme;
private:
	RenderSettings();
	~RenderSettings() {}
};

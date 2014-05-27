#pragma once

#include <map>
#include "linalg.h"
#include "colormap.h"

class RenderSettings
{
public:
	static RenderSettings *inst(bool erase = false);

	void setColors(const OSColors::colorscheme &colors);
	OSColors::colorscheme &getColors();
	OSColors::colorscheme &defaultColorScheme();
	Color4f color(OSColors::RenderColors::RenderColor idx);

	unsigned int openCSGTermLimit, img_width, img_height;
	double far_gl_clip_limit;
private:
	RenderSettings();
	~RenderSettings() {}

	OSColors::colorscheme colors;
};

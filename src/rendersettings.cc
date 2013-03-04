#include "rendersettings.h"

RenderSettings *RenderSettings::inst(bool erase)
{
	static RenderSettings *instance = new RenderSettings;
	if (erase) {
		delete instance;
		instance = NULL;
	}
	return instance;
}

RenderSettings::RenderSettings()
{
	openCSGTermLimit = 2000;
	far_gl_clip_limit = 100000.0;
	img_width = 512;
	img_height = 512;
	this->colors[BACKGROUND_COLOR] = Color4f(0xff, 0xff, 0xe5);
	this->colors[OPENCSG_FACE_FRONT_COLOR] = Color4f(0xf9, 0xd7, 0x2c);
	this->colors[OPENCSG_FACE_BACK_COLOR] = Color4f(0x9d, 0xcb, 0x51);
	this->colors[CGAL_FACE_FRONT_COLOR] = Color4f(0xf9, 0xd7, 0x2c);
	this->colors[CGAL_FACE_BACK_COLOR] = Color4f(0x9d, 0xcb, 0x51);
	this->colors[CGAL_FACE_2D_COLOR] = Color4f(0x00, 0xbf, 0x99);
	this->colors[CGAL_EDGE_FRONT_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colors[CGAL_EDGE_BACK_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colors[CGAL_EDGE_2D_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colors[CROSSHAIR_COLOR] = Color4f(0x80, 0x00, 0x00);
}

Color4f RenderSettings::color(RenderColor idx)
{
	return this->colors[idx];
}

void RenderSettings::setColors(const std::map<RenderColor, Color4f> &colors)
{
	this->colors = colors;
}

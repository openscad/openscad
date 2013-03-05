#ifndef RENDERSETTINGS_H_
#define RENDERSETTINGS_H_

#include <map>
#include "linalg.h"

class RenderSettings
{
public:
	static RenderSettings *inst(bool erase = false);

	enum RenderColor {
		BACKGROUND_COLOR,
		OPENCSG_FACE_FRONT_COLOR,
		OPENCSG_FACE_BACK_COLOR,
		CGAL_FACE_FRONT_COLOR,
		CGAL_FACE_2D_COLOR,
		CGAL_FACE_BACK_COLOR,
		CGAL_EDGE_FRONT_COLOR,
		CGAL_EDGE_BACK_COLOR,
		CGAL_EDGE_2D_COLOR,
		CROSSHAIR_COLOR
	};

	void setColors(const std::map<RenderColor, Color4f> &colors);
	Color4f color(RenderColor idx);

	unsigned int openCSGTermLimit, img_width, img_height;
	double far_gl_clip_limit;
private:
	RenderSettings();
	~RenderSettings() {}

	std::map<RenderColor, Color4f> colors;
};

#endif

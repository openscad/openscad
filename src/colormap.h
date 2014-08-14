#pragma once

#include <map>
#include <string>
#include <list>
#include "linalg.h"
#include <boost/unordered/unordered_map.hpp>

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

typedef std::map<RenderColor, Color4f> ColorScheme;

class ColorMap
{
public:
	static ColorMap *inst(bool erase = false);

	const ColorScheme &defaultColorScheme() const;

	const boost::unordered_map<std::string, Color4f> &webColors() const { return webcolors; }
	const ColorScheme *findColorScheme(const std::string &name) const;
	std::list<std::string> colorSchemeNames() const;

	static Color4f getColor(const ColorScheme &cs, const RenderColor rc);
	
private:
	ColorMap();
	~ColorMap() {}

	boost::unordered_map<std::string, Color4f> webcolors;
	boost::unordered_map<std::string, ColorScheme> colorschemes;
};

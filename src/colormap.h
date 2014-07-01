#pragma once

#include <map>
#include <string>
#include "linalg.h"
#include <boost/unordered/unordered_map.hpp>

namespace OSColors {

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

	typedef std::map<RenderColor, Color4f> colorscheme;

	void init();
	const boost::unordered_map<std::string, Color4f> &webColors();
	const colorscheme *colorScheme(const std::string &name);
	const colorscheme &defaultColorScheme();
	std::list< std::string > colorSchemes();

	Color4f getValue(const colorscheme &cs, const RenderColor rc);
	
}; // namespace OSColors


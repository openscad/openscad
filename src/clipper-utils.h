#ifndef CLIPPER_UTILS_H_
#define CLIPPER_UTILS_H_

#include <polyclipping/clipper.hpp>
#include "Polygon2d.h"

namespace ClipperUtils {

	static const unsigned int CLIPPER_SCALE = 100000;

	ClipperLib::Polygons fromPolygon2d(const class Polygon2d &poly, bool sanitize);
	Polygon2d *toPolygon2d(const ClipperLib::Polygons &poly);

};

#endif

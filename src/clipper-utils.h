#ifndef CLIPPER_UTILS_H_
#define CLIPPER_UTILS_H_

#include "polyclipping/clipper.hpp"
#include "Polygon2d.h"

namespace ClipperUtils {

	static const unsigned int CLIPPER_SCALE = 100000;

	ClipperLib::Path fromOutline2d(const Outline2d &poly);
	ClipperLib::Paths fromPolygon2d(const Polygon2d &poly);
	Polygon2d *toPolygon2d(const ClipperLib::Path &poly);
	Polygon2d *toPolygon2d(const ClipperLib::Paths &poly);
	ClipperLib::Paths process(const ClipperLib::Paths &polygons, 
														ClipperLib::ClipType, ClipperLib::PolyFillType);

};

#endif

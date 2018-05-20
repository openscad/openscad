#pragma once

#include "ext/polyclipping/clipper.hpp"
#include "Polygon2d.h"

namespace ClipperUtils {

	static const unsigned int CLIPPER_SCALE = 1 << 16;

	ClipperLib::Path fromOutline2d(const Outline2d &poly, bool keep_orientation);
	ClipperLib::Paths fromPolygon2d(const Polygon2d &poly);
	ClipperLib::PolyTree sanitize(const ClipperLib::Paths &paths);
	Polygon2d *sanitize(const Polygon2d &poly);
	Polygon2d *toPolygon2d(const ClipperLib::PolyTree &poly);
	ClipperLib::Paths process(const ClipperLib::Paths &polygons, 
														ClipperLib::ClipType, ClipperLib::PolyFillType);
	Polygon2d *applyOffset(const Polygon2d& poly, double offset, ClipperLib::JoinType joinType, double miter_limit, double arc_tolerance);
	Polygon2d *applyMinkowski(const std::vector<const Polygon2d*> &polygons);
	Polygon2d *apply(const std::vector<const Polygon2d*> &polygons, ClipperLib::ClipType);
	Polygon2d *apply(const std::vector<ClipperLib::Paths> &pathsvector, ClipperLib::ClipType);

};

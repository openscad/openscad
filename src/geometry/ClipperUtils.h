#pragma once

#include "ext/polyclipping/clipper.hpp"
#include "Polygon2d.h"

namespace ClipperUtils {

template <typename T>
struct AutoScaled {
  AutoScaled(T&& geom, const BoundingBox& b) : geometry(std::move(geom)), bounds(b) {}
  T geometry;
  BoundingBox bounds;
};

int getScalePow2(const BoundingBox& bounds, int bits = 0);
ClipperLib::Paths fromPolygon2d(const Polygon2d& poly, int pow2);
ClipperLib::PolyTree sanitize(const ClipperLib::Paths& paths);
VectorOfVector2d fromPath(const ClipperLib::Path& path, int pow2);
Polygon2d *sanitize(const Polygon2d& poly);
Polygon2d *toPolygon2d(const ClipperLib::PolyTree& poly, int pow2);
ClipperLib::Paths process(const ClipperLib::Paths& polygons,
                          ClipperLib::ClipType, ClipperLib::PolyFillType);
Polygon2d *applyOffset(const Polygon2d& poly, double offset, ClipperLib::JoinType joinType, double miter_limit, double arc_tolerance);
Polygon2d *applyMinkowski(const std::vector<const Polygon2d *>& polygons);
Polygon2d *apply(const std::vector<const Polygon2d *>& polygons, ClipperLib::ClipType);
}

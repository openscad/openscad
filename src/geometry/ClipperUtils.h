#pragma once

#include "polyclipping/clipper.hpp"
#include "geometry/Polygon2d.h"

#include <memory>
#include <vector>

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
std::unique_ptr<Polygon2d> sanitize(const Polygon2d& poly);
std::unique_ptr<Polygon2d> toPolygon2d(const ClipperLib::PolyTree& poly, int pow2);
ClipperLib::Paths process(const ClipperLib::Paths& polygons,
                          ClipperLib::ClipType, ClipperLib::PolyFillType);
std::unique_ptr<Polygon2d> applyOffset(const Polygon2d& poly, double offset, ClipperLib::JoinType joinType, double miter_limit, double arc_tolerance);
std::unique_ptr<Polygon2d> applyMinkowski(const std::vector<std::shared_ptr<const Polygon2d>>& polygons);
std::unique_ptr<Polygon2d> apply(const std::vector<std::shared_ptr<const Polygon2d>>& polygons, ClipperLib::ClipType);
}

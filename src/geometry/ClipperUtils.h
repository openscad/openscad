#pragma once

#include "clipper2/clipper.h"
#include "Polygon2d.h"

namespace ClipperUtils {

template <typename T>
struct AutoScaled {
  AutoScaled(T&& geom, const BoundingBox& b) : geometry(std::move(geom)), bounds(b) {}
  T geometry;
  BoundingBox bounds;
};

int getScalePow2(const BoundingBox& bounds, int bits = 0);
Clipper2Lib::Paths64 fromPolygon2d(const Polygon2d& poly, int pow2);
std::unique_ptr<Clipper2Lib::PolyTree64> sanitize(const Clipper2Lib::Paths64& paths);
VectorOfVector2d fromPath(const Clipper2Lib::Path64& path, int pow2);
std::unique_ptr<Polygon2d> sanitize(const Polygon2d& poly);
std::unique_ptr<Polygon2d> toPolygon2d(const Clipper2Lib::PolyTree64& poly, int pow2);
Clipper2Lib::Paths64 process(const Clipper2Lib::Paths64& polygons, Clipper2Lib::ClipType, Clipper2Lib::FillRule);
std::unique_ptr<Polygon2d> applyOffset(const Polygon2d& poly, double offset, Clipper2Lib::JoinType joinType, double miter_limit, double arc_tolerance);
std::unique_ptr<Polygon2d> applyMinkowski(const std::vector<std::shared_ptr<const Polygon2d>>& polygons);
std::unique_ptr<Polygon2d> apply(const std::vector<std::shared_ptr<const Polygon2d>>& polygons, Clipper2Lib::ClipType);

}  // namespace ClipperUtils

#pragma once

#include <memory>
#include <vector>

#include "clipper2/clipper.h"
#include "geometry/Polygon2d.h"
#include "geometry/linalg.h"

namespace ClipperUtils {

constexpr int DEFAULT_PRECISION = 8;

int scaleBitsFromBounds(const BoundingBox& bounds, int bits = 0);
int scaleBitsFromPrecision(int precision = DEFAULT_PRECISION);

std::unique_ptr<Clipper2Lib::PolyTree64> sanitize(const Clipper2Lib::Paths64& paths);
std::unique_ptr<Polygon2d> sanitize(const Polygon2d& poly);

Clipper2Lib::Paths64 fromPolygon2d(const Polygon2d& poly, int scale_bits);
Clipper2Lib::Paths64 fromPolygon2d(const Polygon2d& poly, int scale_bits, Color4f *col);
std::unique_ptr<Polygon2d> toPolygon2d(const Clipper2Lib::PolyTree64& poly, int scale_bits);

std::unique_ptr<Polygon2d> applyOffset(const Polygon2d& poly, double offset,
                                       Clipper2Lib::JoinType joinType, double miter_limit,
                                       double arc_tolerance);
std::unique_ptr<Polygon2d> applyMinkowski(const std::vector<std::shared_ptr<const Polygon2d>>& polygons);
std::unique_ptr<Polygon2d> applyProjection(
  const std::vector<std::shared_ptr<const Polygon2d>>& polygons);
std::unique_ptr<Polygon2d> apply(const std::vector<std::shared_ptr<const Polygon2d>>& polygons,
                                 Clipper2Lib::ClipType);
Polygon2d cleanUnion(const std::vector<std::shared_ptr<const Polygon2d>>& polygons);
}  // namespace ClipperUtils

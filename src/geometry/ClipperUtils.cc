#include "geometry/ClipperUtils.h"
#include "geometry/linalg.h"
#include "geometry/Polygon2d.h"
#include "clipper2/clipper.h"
#include "utils/printutils.h"

#include <algorithm>
#include <clipper2/clipper.engine.h>
#include <cmath>
#include <cassert>
#include <utility>
#include <memory>
#include <cstddef>
#include <vector>

namespace ClipperUtils {

namespace {

Clipper2Lib::Paths64 process(const Clipper2Lib::Paths64& polygons, Clipper2Lib::ClipType cliptype,
                             Clipper2Lib::FillRule polytype)
{
  Clipper2Lib::Paths64 result;
  Clipper2Lib::Clipper64 clipper;
  clipper.PreserveCollinear(false);
  clipper.AddSubject(polygons);
  clipper.Execute(cliptype, polytype, result);
  return result;
}

// This is a copy-paste from Clipper2Lib with the modification that the union operation is not performed
// The reason is numeric robustness. With the insides missing, the intersection points created by the
// union operation may (due to rounding) be located at slightly different locations than the original
// geometry and this can give rise to cracks
void minkowski_outline(const Clipper2Lib::Path64& poly, const Clipper2Lib::Path64& path,
                       Clipper2Lib::Paths64& quads, bool isSum, bool isClosed)
{
  int delta = (isClosed ? 1 : 0);
  size_t polyCnt = poly.size();
  size_t pathCnt = path.size();
  Clipper2Lib::Paths64 pp;
  pp.reserve(pathCnt);
  if (isSum)
    for (size_t i = 0; i < pathCnt; ++i) {
      Clipper2Lib::Path64 p;
      p.reserve(polyCnt);
      for (auto point : poly) {
        p.push_back(Clipper2Lib::Point64(path[i].x + point.x, path[i].y + point.y));
      }
      pp.push_back(p);
    }
  else
    for (size_t i = 0; i < pathCnt; ++i) {
      Clipper2Lib::Path64 p;
      p.reserve(polyCnt);
      for (auto point : poly) {
        p.push_back(Clipper2Lib::Point64(path[i].x - point.x, path[i].y - point.y));
      }
      pp.push_back(p);
    }

  quads.reserve((pathCnt + delta) * (polyCnt + 1));
  for (size_t i = 0; i < pathCnt - 1 + delta; ++i)
    for (size_t j = 0; j < polyCnt; ++j) {
      Clipper2Lib::Path64 quad;
      quad.reserve(4);
      quad.push_back(pp[i % pathCnt][j % polyCnt]);
      quad.push_back(pp[(i + 1) % pathCnt][j % polyCnt]);
      quad.push_back(pp[(i + 1) % pathCnt][(j + 1) % polyCnt]);
      quad.push_back(pp[i % pathCnt][(j + 1) % polyCnt]);
      if (!IsPositive(quad)) std::reverse(quad.begin(), quad.end());
      quads.push_back(quad);
    }
}

// Add the polygon a translated to an arbitrary point of each separate component of b.
// Ideally, we would translate to the midpoint of component b, but the point can
// be chosen arbitrarily since the translated object would always stay inside
// the minkowski sum.
void fill_minkowski_insides(const Clipper2Lib::Paths64& a, const Clipper2Lib::Paths64& b,
                            Clipper2Lib::Paths64& target)
{
  for (const auto& b_path : b) {
    // We only need to add for positive components of b
    if (!b_path.empty() && Clipper2Lib::IsPositive(b_path) == 1) {
      const auto& delta = b_path[0];  // arbitrary point
      for (const auto& path : a) {
        target.push_back(path);
        for (auto& point : target.back()) {
          point.x += delta.x;
          point.y += delta.y;
        }
      }
    }
  }
}

void SimplifyPolyTree(const Clipper2Lib::PolyPath64& polytree, double epsilon,
                      Clipper2Lib::PolyPath64& result)
{
  for (const auto& child : polytree) {
    Clipper2Lib::PolyPath64 *newchild =
      result.AddChild(Clipper2Lib::SimplifyPath(child->Polygon(), epsilon));
    SimplifyPolyTree(*child, epsilon, *newchild);
  }
}

}  // namespace

// Using 1 bit less precision than the maximum possible, to limit the chance
// of data loss when converting back to double (see https://github.com/openscad/openscad/issues/5253).
const int CLIPPER_BITS{std::ilogb(0x3FFFFFFFFFFFFFFFLL)};

int scaleBitsFromBounds(const BoundingBox& bounds, int total_bits)
{
  const double maxCoeff = std::max(
    {bounds.min().cwiseAbs().maxCoeff(), bounds.max().cwiseAbs().maxCoeff(), bounds.sizes().maxCoeff()});
  const int exp = std::ilogb(maxCoeff) + 1;
  const int actual_bits = (total_bits == 0) ? CLIPPER_BITS : total_bits;
  return (actual_bits - 1) - exp;
}

int scaleBitsFromPrecision(int precision) { return std::ilogb(std::pow(10, precision)) + 1; }

Clipper2Lib::Paths64 fromPolygon2d(const Polygon2d& poly, int scale_bits)
{
  const bool keep_orientation = poly.isSanitized();
  const double scale = std::ldexp(1.0, scale_bits);
  Clipper2Lib::Paths64 result;
  for (const auto& outline : poly.outlines()) {
    Clipper2Lib::Path64 p;
    for (const auto& v : outline.vertices) {
      p.emplace_back(v[0] * scale, v[1] * scale);
    }
    // Make sure all polygons point up, since we project also
    // back-facing polygon in PolySetUtils::project()
    if (!keep_orientation && !Clipper2Lib::IsPositive(p)) std::reverse(p.begin(), p.end());
    result.push_back(std::move(p));
  }
  return result;
}

Clipper2Lib::Paths64 fromPolygon2d(const Polygon2d& poly)
{
  return fromPolygon2d(poly, scaleBitsFromPrecision());
}

std::unique_ptr<Clipper2Lib::PolyTree64> sanitize(const Clipper2Lib::Paths64& paths)
{
  auto result = std::make_unique<Clipper2Lib::PolyTree64>();
  Clipper2Lib::Clipper64 clipper;
  clipper.PreserveCollinear(false);
  try {
    clipper.AddSubject(paths);
  } catch (...) {
    // Most likely caught a RangeTest exception from clipper
    // Note that Clipper up to v6.2.1 incorrectly throws
    // an exception of type char* rather than a clipperException()
    // TODO: Is this needed for Clipper2?
    LOG(message_group::Warning, "Range check failed for polygon. skipping");
  }
  clipper.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::EvenOdd, *result);
  return result;
}

std::unique_ptr<Polygon2d> sanitize(const Polygon2d& poly)
{
  auto scale_bits = scaleBitsFromPrecision();

  auto paths = ClipperUtils::fromPolygon2d(poly, scale_bits);
  return toPolygon2d(*sanitize(paths), scale_bits);
}

/*!
   We want to use a PolyTree to convert to Polygon2d, since only PolyTrees
   have an explicit notion of holes.
   We could use a Paths structure, but we'd have to check the orientation of each
   path before adding it to the Polygon2d.
 */
std::unique_ptr<Polygon2d> toPolygon2d(const Clipper2Lib::PolyTree64& polytree, int scale_bits)
{
  auto result = std::make_unique<Polygon2d>();
  const double scale = std::ldexp(1.0, -scale_bits);
  auto processChildren = [scale, &result](auto&& processChildren,
                                          const Clipper2Lib::PolyPath64& node) -> void {
    Outline2d outline;
    // When using offset, clipper can get the hole status wrong.
    // IsPositive() calculates the area of the polygon, and if it's negative, it's a hole.
    outline.positive = IsPositive(node.Polygon());

    constexpr double epsilon = 1.1415;  // Epsilon taken from Clipper1's default epsilon.
    const auto cleaned_path = Clipper2Lib::SimplifyPath(node.Polygon(), epsilon);

    // SimplifyPath can potentially reduce the polygon down to no vertices
    if (cleaned_path.size() >= 3) {
      for (const auto& ip : cleaned_path) {
        outline.vertices.emplace_back(scale * ip.x, scale * ip.y);
      }
      result->addOutline(outline);
    }
    for (const auto& child : node) {
      processChildren(processChildren, *child);
    }
  };
  for (const auto& node : polytree) {
    processChildren(processChildren, *node);
  }
  result->setSanitized(true);
  return result;
}

/*!
   Apply the clipper operator to the given paths.

   May return an empty Polygon2d, but will not return nullptr.
 */
std::unique_ptr<Polygon2d> apply(const std::vector<Clipper2Lib::Paths64>& pathsvector,
                                 Clipper2Lib::ClipType clipType, int scale_bits)
{
  Clipper2Lib::Clipper64 clipper;
  clipper.PreserveCollinear(false);

  if (clipType == Clipper2Lib::ClipType::Intersection && pathsvector.size() >= 2) {
    // intersection operations must be split into a sequence of binary operations
    auto source = pathsvector[0];
    Clipper2Lib::PolyTree64 result;
    for (unsigned int i = 1; i < pathsvector.size(); ++i) {
      clipper.AddSubject(source);
      clipper.AddClip(pathsvector[i]);
      clipper.Execute(clipType, Clipper2Lib::FillRule::NonZero, result);
      if (i != pathsvector.size() - 1) {
        source = Clipper2Lib::PolyTreeToPaths64(result);
        clipper.Clear();
      }
    }
    return ClipperUtils::toPolygon2d(result, scale_bits);
  }

  bool first = true;
  for (const auto& paths : pathsvector) {
    if (first) {
      clipper.AddSubject(paths);
      first = false;
    } else {
      clipper.AddClip(paths);
    }
  }
  Clipper2Lib::PolyTree64 sumresult;
  clipper.Execute(clipType, Clipper2Lib::FillRule::NonZero, sumresult);
  // The returned result will have outlines ordered according to whether
  // they're positive or negative: Positive outlines counter-clockwise and
  // negative outlines clockwise.
  return ClipperUtils::toPolygon2d(sumresult, scale_bits);
}

/*!
   Apply the clipper operator to the given polygons.

   May return an empty Polygon2d, but will not return nullptr.
 */
std::unique_ptr<Polygon2d> apply(const std::vector<std::shared_ptr<const Polygon2d>>& polygons,
                                 Clipper2Lib::ClipType clipType)
{
  const int scale_bits = scaleBitsFromPrecision();

  std::vector<Clipper2Lib::Paths64> pathsvector;
  for (const auto& polygon : polygons) {
    if (polygon) {
      auto polypaths = fromPolygon2d(*polygon, scale_bits);
      if (!polygon->isSanitized()) {
        polypaths = Clipper2Lib::PolyTreeToPaths64(*sanitize(polypaths));
      }
      pathsvector.push_back(std::move(polypaths));
    } else {
      // Insert empty object as this could be the positive object in a difference
      pathsvector.emplace_back();
    }
  }
  auto res = apply(pathsvector, clipType, scale_bits);
  assert(res);
  return res;
}

std::unique_ptr<Polygon2d> applyMinkowski(const std::vector<std::shared_ptr<const Polygon2d>>& polygons)
{
  if (polygons.size() == 1) {
    return polygons[0] ? std::make_unique<Polygon2d>(*polygons[0]) : nullptr;  // Just copy
  }

  auto it = polygons.begin();
  while (it != polygons.end() && !(*it)) ++it;
  if (it == polygons.end()) return nullptr;
  const int scale_bits = scaleBitsFromPrecision();

  Clipper2Lib::Clipper64 clipper;
  clipper.PreserveCollinear(false);
  auto lhs = fromPolygon2d(polygons[0] ? *polygons[0] : Polygon2d(), scale_bits);

  for (size_t i = 1; i < polygons.size(); ++i) {
    if (!polygons[i]) continue;
    Clipper2Lib::Paths64 minkowski_terms;
    auto rhs = fromPolygon2d(*polygons[i], scale_bits);

    // First, convolve each outline of lhs with the outlines of rhs
    for (auto const& rhs_path : rhs) {
      for (auto const& lhs_path : lhs) {
        Clipper2Lib::Paths64 result;
        minkowski_outline(lhs_path, rhs_path, result, true, true);
        minkowski_terms.insert(minkowski_terms.end(), result.begin(), result.end());
      }
    }

    // Then, fill the central parts
    fill_minkowski_insides(lhs, rhs, minkowski_terms);
    fill_minkowski_insides(rhs, lhs, minkowski_terms);

    // This union operation must be performed at each iteration since the minkowski_terms
    // now contain lots of small quads
    clipper.Clear();
    clipper.AddSubject(minkowski_terms);

    if (i != polygons.size() - 1) {
      clipper.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::NonZero, lhs);
    }
  }

  Clipper2Lib::PolyTree64 polytree;
  clipper.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::NonZero, polytree);
  return toPolygon2d(polytree, scale_bits);
}

std::unique_ptr<Polygon2d> applyOffset(const Polygon2d& poly, double offset,
                                       Clipper2Lib::JoinType joinType, double miter_limit,
                                       double arc_tolerance)
{
  const bool isMiter = joinType == Clipper2Lib::JoinType::Miter;
  const bool isRound = joinType == Clipper2Lib::JoinType::Round;
  const int scale_bits = scaleBitsFromPrecision();
  Clipper2Lib::ClipperOffset co(isMiter ? miter_limit : 2.0,
                                isRound ? std::ldexp(arc_tolerance, scale_bits) : 1.0);
  auto p = ClipperUtils::fromPolygon2d(poly, scale_bits);
  co.AddPaths(p, joinType, Clipper2Lib::EndType::Polygon);
  Clipper2Lib::PolyTree64 result;
  co.Execute(std::ldexp(offset, scale_bits), result);
  return toPolygon2d(result, scale_bits);
}

std::unique_ptr<Polygon2d> applyProjection(const std::vector<std::shared_ptr<const Polygon2d>>& polygons)
{
  const int scale_bits = scaleBitsFromPrecision();

  Clipper2Lib::Clipper64 sumclipper;
  sumclipper.PreserveCollinear(false);
  for (const auto& poly : polygons) {
    Clipper2Lib::Paths64 result = ClipperUtils::fromPolygon2d(*poly, scale_bits);
    // Using NonZero ensures that we don't create holes from polygons sharing
    // edges since we're unioning a mesh
    result = ClipperUtils::process(result, Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::NonZero);
    // Add correctly winded polygons to the main clipper
    sumclipper.AddSubject(result);
  }

  Clipper2Lib::PolyTree64 sumresult;
  // This is key - without StrictlySimple, we tend to get self-intersecting results
  // FIXME: StrictlySimple doesn't exist in Clipper2. Check if it still exposes problems without
  //  sumclipper.StrictlySimple(true);
  sumclipper.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::NonZero, sumresult);
  if (sumresult.Count() > 0) {
    return ClipperUtils::toPolygon2d(sumresult, scale_bits);
  }
  return {};
}

}  // namespace ClipperUtils

#include "ClipperUtils.h"
#include "printutils.h"

namespace ClipperUtils {

const int CLIPPER_BITS{ std::ilogb( 0x3FFFFFFFFFFFFFFFLL) };

int getScalePow2(const BoundingBox& bounds, int bits)
{
  double maxCoeff = std::max({
      bounds.min().cwiseAbs().maxCoeff(),
      bounds.max().cwiseAbs().maxCoeff(),
      bounds.sizes().maxCoeff()
    });
  int exp = std::ilogb(maxCoeff) + 1;
  int _bits = (bits == 0) ? CLIPPER_BITS : bits;
  return (_bits - 1) - exp;
}

VectorOfVector2d fromPath(const Clipper2Lib::Path64& path, int pow2)
{
  double scale = std::ldexp(1.0, -pow2);
  VectorOfVector2d ret;
  for (auto v : path) {
    ret.emplace_back(v.x * scale, v.y * scale);
  }
  return ret;
}

Clipper2Lib::Paths64 fromPolygon2d(const Polygon2d& poly, int pow2)
{
  bool keep_orientation = poly.isSanitized();
  double scale = std::ldexp(1.0, pow2);
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

AutoScaled<Clipper2Lib::Paths64> fromPolygon2d(const Polygon2d& poly)
{
  auto b = poly.getBoundingBox();
  auto scale = ClipperUtils::getScalePow2(b);
  return {fromPolygon2d(poly, scale), b};
}

std::unique_ptr<Clipper2Lib::PolyTree64> sanitize(const Clipper2Lib::Paths64& paths)
{
  auto result = std::make_unique<Clipper2Lib::PolyTree64>();
  Clipper2Lib::Clipper64 clipper;
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
  auto tmp = ClipperUtils::fromPolygon2d(poly);
  return toPolygon2d(*sanitize(tmp.geometry), ClipperUtils::getScalePow2(tmp.bounds));
}

/*!
   We want to use a PolyTree to convert to Polygon2d, since only PolyTrees
   have an explicit notion of holes.
   We could use a Paths structure, but we'd have to check the orientation of each
   path before adding it to the Polygon2d.
 */
std::unique_ptr<Polygon2d> toPolygon2d(const Clipper2Lib::PolyTree64& polytree, int pow2)
{
  auto result = std::make_unique<Polygon2d>();
  double scale = std::ldexp(1.0, -pow2);

  auto processChildren = [scale, &result](auto&& processChildren, const Clipper2Lib::PolyPath64& node) -> void {
    Outline2d outline;
    // Apparently, when using offset(), clipper gets the hole status wrong
    //outline.positive = !node->IsHole();
    outline.positive = IsPositive(node.Polygon());
    // TODO: Should we replace the missing CleanPolygons in Clipper2 and call it here?
    // CleanPolygon can in some cases reduce the polygon down to no vertices
    const auto &cleaned_path = node.Polygon();
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

Clipper2Lib::Paths64 process(const Clipper2Lib::Paths64& polygons,
                          Clipper2Lib::ClipType cliptype,
                          Clipper2Lib::FillRule polytype)
{
  Clipper2Lib::Paths64 result;
  Clipper2Lib::Clipper64 clipper;
  clipper.AddSubject(polygons);
  clipper.Execute(cliptype, polytype, result);
  return result;
}

/*!
   Apply the clipper operator to the given paths.

   May return an empty Polygon2d, but will not return nullptr.
 */
std::unique_ptr<Polygon2d> apply(const std::vector<Clipper2Lib::Paths64>& pathsvector,
				 Clipper2Lib::ClipType clipType, int pow2)
{
  Clipper2Lib::Clipper64 clipper;

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
    return ClipperUtils::toPolygon2d(result, pow2);
  }

  bool first = true;
  for (const auto& paths : pathsvector) {
    if (first) {
      clipper.AddSubject(paths);
      first = false;
    }
    else {
      clipper.AddClip(paths);
    }
  }
  Clipper2Lib::PolyTree64 sumresult;
  clipper.Execute(clipType, Clipper2Lib::FillRule::NonZero, sumresult);
  // The returned result will have outlines ordered according to whether
  // they're positive or negative: Positive outlines counter-clockwise and
  // negative outlines clockwise.
  return ClipperUtils::toPolygon2d(sumresult, pow2);
}

/*!
   Apply the clipper operator to the given polygons.

   May return an empty Polygon2d, but will not return nullptr.
 */
std::unique_ptr<Polygon2d> apply(const std::vector<std::shared_ptr<const Polygon2d>>& polygons,
				 Clipper2Lib::ClipType clipType)
{
  BoundingBox bounds;
  for (const auto &polygon : polygons) {
    if (polygon) bounds.extend(polygon->getBoundingBox());
  }
  int pow2 = ClipperUtils::getScalePow2(bounds);

  std::vector<Clipper2Lib::Paths64> pathsvector;
  for (const auto& polygon : polygons) {
    if (polygon) {
      auto polypaths = fromPolygon2d(*polygon, pow2);
      if (!polygon->isSanitized()) {
        polypaths = Clipper2Lib::PolyTreeToPaths64(*sanitize(polypaths));
      }
      pathsvector.push_back(std::move(polypaths));
    } else {
      // Insert empty object as this could be the positive object in a difference
      pathsvector.emplace_back();
    }
  }
  auto res = apply(pathsvector, clipType, pow2);
  assert(res);
  return res;
}

// This is a copy-paste from ClipperLib with the modification that the union operation is not performed
// The reason is numeric robustness. With the insides missing, the intersection points created by the union operation may
// (due to rounding) be located at slightly different locations than the original geometry and this
// can give rise to cracks
static void minkowski_outline(const Clipper2Lib::Path64& poly, const Clipper2Lib::Path64& path,
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
static void fill_minkowski_insides(const Clipper2Lib::Paths64& a,
                                   const Clipper2Lib::Paths64& b,
                                   Clipper2Lib::Paths64& target)
{
  for (const auto& b_path : b) {
    // We only need to add for positive components of b
    if (!b_path.empty() && Clipper2Lib::IsPositive(b_path) == 1) {
      const auto& delta = b_path[0]; // arbitrary point
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

std::unique_ptr<Polygon2d> applyMinkowski(const std::vector<std::shared_ptr<const Polygon2d>>& polygons)
{
  if (polygons.size() == 1) {
    return polygons[0] ? std::make_unique<Polygon2d>(*polygons[0]) : nullptr; // Just copy
  }

  auto it = polygons.begin();
  while (it != polygons.end() && !(*it)) ++it;
  if (it == polygons.end()) return nullptr;
  BoundingBox out_bounds = (*it)->getBoundingBox();
  BoundingBox in_bounds;
  for ( ; it != polygons.end(); ++it) {
    if (*it) {
      auto tmp = (*it)->getBoundingBox();
      in_bounds.extend(tmp);
      out_bounds.min() += tmp.min();
      out_bounds.max() += tmp.max();
    }
  }
  int pow2 = getScalePow2(in_bounds.extend(out_bounds));

  Clipper2Lib::Clipper64 clipper;
  auto lhs = fromPolygon2d(polygons[0] ? *polygons[0] : Polygon2d(), pow2);

  for (size_t i = 1; i < polygons.size(); ++i) {
    if (!polygons[i]) continue;
    Clipper2Lib::Paths64 minkowski_terms;
    auto rhs = fromPolygon2d(*polygons[i], pow2);

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
  return toPolygon2d(polytree, pow2);
}

std::unique_ptr<Polygon2d> applyOffset(const Polygon2d& poly, double offset, Clipper2Lib::JoinType joinType,
				       double miter_limit, double arc_tolerance)
{
  bool isMiter = joinType == Clipper2Lib::JoinType::Miter;
  bool isRound = joinType == Clipper2Lib::JoinType::Round;
  auto bounds = poly.getBoundingBox();
  double max_diff = std::abs(offset) * (isMiter ? miter_limit : 1.0);
  bounds.min() -= Vector3d(max_diff, max_diff, 0);
  bounds.max() += Vector3d(max_diff, max_diff, 0);
  int pow2 = getScalePow2(bounds);
  Clipper2Lib::ClipperOffset co(
    isMiter ? miter_limit : 2.0,
    isRound ? std::ldexp(arc_tolerance, pow2) : 1.0
    );
  auto p = ClipperUtils::fromPolygon2d(poly, pow2);
  co.AddPaths(p, joinType, Clipper2Lib::EndType::Polygon);
  Clipper2Lib::PolyTree64 result;
  co.Execute(std::ldexp(offset, pow2), result);
  return toPolygon2d(result, pow2);
}
} // namespace ClipperUtils

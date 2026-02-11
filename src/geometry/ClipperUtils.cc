#include "geometry/ClipperUtils.h"

#include <clipper2/clipper.engine.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "clipper2/clipper.h"
#include "geometry/Polygon2d.h"
#include "geometry/linalg.h"
#include "utils/printutils.h"

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

int scaleBitsFromPrecision(int precision)
{
  return std::ilogb(std::pow(10, precision)) + 1;
}

Clipper2Lib::Paths64 fromPolygon2d(const Polygon2d& poly, int scale_bits, Color4f *col)
{
  const bool keep_orientation = poly.isSanitized();
  const double scale = std::ldexp(1.0, scale_bits);
  Clipper2Lib::Paths64 result;
  for (const auto& outline : poly.transformedOutlines()) {
    Clipper2Lib::Path64 p;
    if (col == nullptr || *col == outline.color) {
      for (const auto& v : outline.vertices) {
        p.emplace_back(v[0] * scale, v[1] * scale);
      }
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
  return fromPolygon2d(poly, scaleBitsFromPrecision(), nullptr);
}

Clipper2Lib::Paths64 fromPolygon2d(const Polygon2d& poly, Color4f *col)
{
  return fromPolygon2d(poly, scaleBitsFromPrecision(), col);
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

  auto paths = ClipperUtils::fromPolygon2d(poly, scale_bits, nullptr);
  auto result = toPolygon2d(*sanitize(paths), scale_bits);
  result->stamp_color(poly);
  return result;
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

std::unique_ptr<Polygon2d> apply(const std::vector<Clipper2Lib::Paths64>& pathsvector,
                                 Clipper2Lib::ClipType clipType, int scale_bits);

Polygon2d cleanUnion(const std::vector<std::shared_ptr<const Polygon2d>>& polygons)
{
  int n = polygons.size();

  const int scale_bits = scaleBitsFromPrecision();

  std::vector<Clipper2Lib::Paths64> pathsvector;
  for (const auto& polygon : polygons) {
    if (polygon) {
      auto polypaths = fromPolygon2d(*polygon, scale_bits, nullptr);
      if (!polygon->isSanitized()) {
        polypaths = Clipper2Lib::PolyTreeToPaths64(*sanitize(polypaths));
      }
      pathsvector.push_back(std::move(polypaths));
    } else {
      // Insert empty object as this could be the positive object in a difference
      pathsvector.emplace_back();
    }
  }

  int inputs_old = 0;

  Polygon2d union_result;
  while (inputs_old < n) {
    if (polygons[inputs_old] == nullptr) {
      inputs_old++;
      continue;
    }

    int input_width = 1;
    while (inputs_old + input_width < n) {
      if (polygons[inputs_old]->outlines().size() > 1) break;
      if (polygons[inputs_old + input_width] == nullptr) break;
      if (polygons[inputs_old + input_width]->outlines().size() > 1) break;
      if (polygons[inputs_old]->outlines()[0].color !=
          polygons[inputs_old + input_width]->outlines()[0].color)
        break;
      input_width++;
    }
    auto cur_outlines = polygons[inputs_old]->outlines();
    while (cur_outlines.size() > 0) {
      Color4f curcol = cur_outlines[0].color;

      // union all new shapes
      Clipper2Lib::Paths64 union_new;
      if (input_width > 1) {
        std::vector<Clipper2Lib::Paths64> union_operands;
        for (int i = 0; i < input_width; i++) {
          auto polypath = fromPolygon2d(*polygons[inputs_old + i], scale_bits, &curcol);
          union_operands.push_back(polypath);
        }
        std::unique_ptr<Polygon2d> union_result =
          apply(union_operands, Clipper2Lib::ClipType::Union, scale_bits);
        union_new = fromPolygon2d(*union_result, scale_bits, nullptr);
      } else {
        union_new = fromPolygon2d(*polygons[inputs_old], scale_bits, &curcol);
      }
      if (!polygons[inputs_old]->isSanitized()) {
        union_new = Clipper2Lib::PolyTreeToPaths64(*sanitize(union_new));
      }

      // difference all old

      std::vector<Clipper2Lib::Paths64> diff_operands;
      diff_operands.push_back(union_new);
      for (int i = 0; i < inputs_old; i++) {
        if (polygons[i] == nullptr) continue;
        diff_operands.push_back(pathsvector[i]);  // only when its different color
      }
      std::unique_ptr<Polygon2d> diff_result =
        apply(diff_operands, Clipper2Lib::ClipType::Difference, scale_bits);
      for (Outline2d o : diff_result->outlines()) {
        o.color = curcol;
        union_result.addOutline(o);
      }
      std::vector<Outline2d> new_outlines;  // now create new vector with this color removed
      for (const auto& o : cur_outlines) {
        if (o.color == curcol) continue;
        new_outlines.push_back(o);
      }
      cur_outlines = new_outlines;
    }

    inputs_old += input_width;
  }

  // per-color sanitizing

  std::vector<Outline2d> outlines = union_result.outlines();
  Polygon2d union_sanitized;
  while (outlines.size() > 0) {
    Color4f refcol = outlines[0].color;
    std::vector<Outline2d> outlines_todo;
    Polygon2d outlines_singlecol;
    for (const auto& ol : outlines) {
      if (ol.color == refcol) {
        outlines_singlecol.addOutline(ol);
      } else outlines_todo.push_back(ol);
    }
    std::shared_ptr<Polygon2d> sanitized = sanitize(outlines_singlecol);
    for (Outline2d ol : sanitized->outlines()) {
      ol.color = refcol;
      union_sanitized.addOutline(ol);
    }
    outlines = outlines_todo;
  }
  union_result = union_sanitized;
  union_result.setSanitized(true);
  return union_result;
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
#ifdef ENABLE_MANIFOLD
  // only create colored data for manifold, cgal tessellator cannot handle it later

  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
    if (clipType == Clipper2Lib::ClipType::Union) {
      Polygon2d union_result = cleanUnion(polygons);
      return std::make_unique<Polygon2d>(union_result);
    }
  }
#endif
  Polygon2d result;
  std::vector<Outline2d> outlines_work;

  std::vector<Clipper2Lib::Paths64> pathsvector_diff;
  bool first = true;
  for (const auto& polygon : polygons) {
    if (clipType == Clipper2Lib::ClipType::Union && polygon == nullptr) continue;
    // prepare subject
    if (first) {
      if (polygon != nullptr) outlines_work = polygon->outlines();
      first = false;
      continue;
    }
    if (polygon == nullptr) continue;

    // convert diff section
    if (polygon) {
      auto polypaths = fromPolygon2d(*polygon, scale_bits, nullptr);
      if (!polygon->isSanitized()) {
        polypaths = Clipper2Lib::PolyTreeToPaths64(*sanitize(polypaths));
      }
      pathsvector_diff.push_back(std::move(polypaths));
    } else {
      // Insert empty object as this could be the positive object in a difference
      pathsvector_diff.emplace_back();
    }
  }
  while (outlines_work.size() > 0) {
    Color4f col = outlines_work[0].color;
    Polygon2d polygon_cur;
    std::vector<Outline2d> outlines_new;

    for (const auto& outl : outlines_work) {
      if (outl.color == col) polygon_cur.addOutline(outl);
      else outlines_new.push_back(outl);
    }
    polygon_cur.setSanitized(true);

    // prepare diff for one color
    std::vector<Clipper2Lib::Paths64> pathsvector;

    // subject
    auto polypaths = fromPolygon2d(polygon_cur, scale_bits, nullptr);
    if (!polygon_cur.isSanitized()) {
      polypaths = Clipper2Lib::PolyTreeToPaths64(*sanitize(polypaths));
    }
    pathsvector.push_back(std::move(polypaths));
    // and the rest

    for (auto diff : pathsvector_diff) {
      pathsvector.push_back(diff);
    }

    auto res = apply(pathsvector, clipType, scale_bits);
    for (auto out : res->outlines()) {  // collect result
      out.color = col;
      result.addOutline(out);
    }
    assert(res);
    outlines_work = outlines_new;
  }
  result.setSanitized(true);
  return std::make_unique<Polygon2d>(result);
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
  auto lhs = fromPolygon2d(polygons[0] ? *polygons[0] : Polygon2d(), scale_bits, nullptr);

  for (size_t i = 1; i < polygons.size(); ++i) {
    if (!polygons[i]) continue;
    Clipper2Lib::Paths64 minkowski_terms;
    auto rhs = fromPolygon2d(*polygons[i], scale_bits, nullptr);

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
  auto result = toPolygon2d(polytree, scale_bits);
  result->setColor(*OpenSCAD::parse_color("#f9d72c"));
  return result;
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
  auto p = ClipperUtils::fromPolygon2d(poly, scale_bits, nullptr);
  co.AddPaths(p, joinType, Clipper2Lib::EndType::Polygon);
  Clipper2Lib::PolyTree64 result;
  co.Execute(std::ldexp(offset, scale_bits), result);
  auto r = toPolygon2d(result, scale_bits);
  r->transform3d(poly.getTransform3d());
  r->stamp_color(poly);
  return r;
}

std::unique_ptr<Polygon2d> applyProjection(const std::vector<std::shared_ptr<const Polygon2d>>& polygons)
{
  const int scale_bits = scaleBitsFromPrecision();

  Clipper2Lib::Clipper64 sumclipper;
  sumclipper.PreserveCollinear(false);
  for (const auto& poly : polygons) {
    Clipper2Lib::Paths64 result = ClipperUtils::fromPolygon2d(*poly, scale_bits, nullptr);
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
    auto res = ClipperUtils::toPolygon2d(sumresult, scale_bits);
    for (const auto& poly : polygons) {
      res->stamp_color(*poly);
    }
    return res;
  }
  return {};
}

}  // namespace ClipperUtils

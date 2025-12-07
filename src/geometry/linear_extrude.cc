#include "geometry/linear_extrude.h"

#include <algorithm>
#include <iterator>
#include <cassert>
#include <cmath>
#include <utility>
#include <memory>
#include <cstddef>
#include <vector>

#include <boost/logic/tribool.hpp>

#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/GeometryUtils.h"
#include "glview/RenderSettings.h"
#include "core/LinearExtrudeNode.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "utils/degree_trig.h"

namespace LinearExtrudeInternals {

/*
  Compare Euclidean length of vectors
  Return:
   -1 : if v1  < v2
    0 : if v1 ~= v2 (approximation to compoensate for floating point precision)
    1 : if v1  > v2
*/
int sgn_vdiff(const Vector2d& v1, const Vector2d& v2)
{
  constexpr double ratio_threshold = 1e5;  // 10ppm difference
  double l1 = v1.norm();
  double l2 = v2.norm();
  // Compare the average and difference, to be independent of geometry scale.
  // If the difference is within ratio_threshold of the avg, treat as equal.
  double scale = (l1 + l2) * ratio_threshold;
  double diff = 2 * std::fabs(l1 - l2);
  return scale < diff ? (l1 < l2 ? -1 : 1) : 0;
}

/**
 *
 * @param vertices The first polyref.length() vertices must be in the same order as polyref and represent
 * the bottom face. Similarly, the last polyref.length() vertices must be in the same order as polyref
 * and represent the top face.
 * @param indices These indexed face sets must not include the top nor bottom faces.
 */
std::unique_ptr<PolySet> assemblePolySetForManifold(const Polygon2d& polyref,
                                                    std::vector<Vector3d>&& vertices,
                                                    PolygonIndices&& indices, int convexity,
                                                    boost::tribool isConvex, int index_offset)
{
  auto final_polyset = std::make_unique<PolySet>(3, isConvex);
  final_polyset->setTriangular(true);
  final_polyset->setConvexity(convexity);
  final_polyset->vertices = std::move(vertices);
  final_polyset->indices = std::move(indices);

  // Get valid top and bottom edges (as Indexed Face Sets aka Indices).
  // If top is scaled it doesn't matter: we're only using the edges and not vertices.
  auto ps_topbottom = polyref.tessellate();
  // Manifold tessellating doesn't add vertices (at least in this case? ever? not sure), so the indices
  // remain valid.

  // Copy indices for the top face, with appropriate offset.
  for (const auto& p_original : ps_topbottom->indices) {
    final_polyset->indices.emplace_back();
    auto& p_offset = final_polyset->indices.back();

    for (int index : p_original) {
      p_offset.push_back(index + index_offset);
    }
  }

  // Flip vertex ordering for bottom polygon and move it.
  for (auto& p : ps_topbottom->indices) {
    std::reverse(p.begin(), p.end());
  }
  std::copy(std::make_move_iterator(ps_topbottom->indices.begin()),
            std::make_move_iterator(ps_topbottom->indices.end()),
            std::back_inserter(final_polyset->indices));

  // LOG(PolySetUtils::polySetToPolyhedronSource(*final_polyset));

  return final_polyset;
}

std::unique_ptr<PolySet> assemblePolySetForCGAL(const Polygon2d& polyref,
                                                std::vector<Vector3d>& vertices, PolygonIndices& indices,
                                                int convexity, boost::tribool isConvex, double scale_x,
                                                double scale_y, const Vector3d& h1, const Vector3d& h2,
                                                double twist)
{
  PolySetBuilder builder(0, 0, 3, isConvex);
  builder.setConvexity(convexity);

  for (const auto& poly : indices) {
    builder.beginPolygon(poly.size());
    for (const auto idx : poly) {
      builder.addVertex(vertices[idx]);
    }
  }

  auto translatePolySet = [](PolySet& ps, const Vector3d& translation) {
    for (auto& v : ps.vertices) {
      v += translation;
    }
  };

  // Create bottom face.
  auto ps_bottom = polyref.tessellate();  // bottom
  // Flip vertex ordering for bottom polygon
  for (auto& p : ps_bottom->indices) {
    std::reverse(p.begin(), p.end());
  }
  translatePolySet(*ps_bottom, h1);
  builder.appendPolySet(*ps_bottom);

  // Create top face.
  // If either scale components are 0, then top will be zero-area, so skip it.
  if (scale_x != 0 && scale_y != 0) {
    Polygon2d top_poly(polyref);
    Eigen::Affine2d trans(Eigen::Scaling(scale_x, scale_y) * Eigen::Affine2d(rotate_degrees(-twist)));
    top_poly.transform(trans);
    auto ps_top = top_poly.tessellate();
    translatePolySet(*ps_top, h2);
    builder.appendPolySet(*ps_top);
  }

  return builder.build();
}

/*
   Attempt to triangulate quads in an ideal way.
   Each quad is composed of two adjacent outline vertices: (prev1, curr1)
   and their corresponding transformed points one step up: (prev2, curr2).
   Quads are triangulated across the shorter of the two diagonals, which works well in most cases.
   However, when diagonals are equal length, decision may flip depending on other factors.
 */
void add_slice_indices(PolygonIndices& indices, int slice_idx, int slice_stride, const Polygon2d& poly,
                       double rot1, double rot2, const Vector2d& scale1, const Vector2d& scale2)
{
  int prev_slice = (slice_idx - 1) * slice_stride;
  int curr_slice = slice_idx * slice_stride;

  Eigen::Affine2d trans1(Eigen::Scaling(scale1) * Eigen::Affine2d(rotate_degrees(-rot1)));
  Eigen::Affine2d trans2(Eigen::Scaling(scale2) * Eigen::Affine2d(rotate_degrees(-rot2)));

  bool any_zero = scale2[0] == 0 || scale2[1] == 0;
  // setting back_twist true helps keep diagonals same as previous builds.
  bool back_twist = rot2 <= rot1;

  int curr_outline = 0;
  for (const auto& o : poly.outlines()) {
    // prev1: previous slice, previous vertex
    // prev2: current slice, previous vertex
    Vector2d prev1 = trans1 * o.vertices[0];
    Vector2d prev2 = trans2 * o.vertices[0];

    // For equal length diagonals, flip selected choice depending on direction of twist and
    // whether the outline is negative (eg circle hole inside a larger circle).
    // This was tested against circles with a single point touching the origin,
    // and extruded with twist.  Diagonal choice determined by whichever option
    // matched the direction of diagonal for neighboring edges (which did not exhibit "equal" diagonals).
    bool flip = ((!o.positive) xor (back_twist));

    for (size_t i = 1; i <= o.vertices.size(); ++i) {
      // curr1: previous slice, current vertex
      // curr2: current slice, current vertex
      Vector2d curr1 = trans1 * o.vertices[i % o.vertices.size()];
      Vector2d curr2 = trans2 * o.vertices[i % o.vertices.size()];
      int curr_idx = curr_outline + (i % o.vertices.size());
      int prev_idx = curr_outline + i - 1;

      int diff_sign = sgn_vdiff(prev1 - curr2, curr1 - prev2);
      bool splitfirst = diff_sign == -1 || (diff_sign == 0 && !flip);

      // Split along shortest diagonal,
      // unless at top for a 0-scaled axis (which can create 0 thickness "ears")
      if (splitfirst xor any_zero) {
        indices.push_back({
          prev_slice + curr_idx,
          curr_slice + curr_idx,
          prev_slice + prev_idx,
        });
        indices.push_back({
          curr_slice + prev_idx,
          prev_slice + prev_idx,
          curr_slice + curr_idx,
        });
      } else {
        indices.push_back({
          prev_slice + curr_idx,
          curr_slice + prev_idx,
          prev_slice + prev_idx,
        });
        indices.push_back({
          prev_slice + curr_idx,
          curr_slice + curr_idx,
          curr_slice + prev_idx,
        });
      }
      prev1 = curr1;
      prev2 = curr2;
    }
    curr_outline += o.vertices.size();
  }
}

/**
 *  max(2Ddistance(vertex->scaled_vertex)^2 of all vertices).
 */
inline double calc_max_delta_sqr(const std::vector<Outline2d>& outlines, const Vector2d& scale)
{
  double max_delta_sqr = 0;
  for (const auto& o : outlines) {
    for (const auto& v : o.vertices) {
      // cwiseProduct means multiplying each element by the same element in the other matrix
      // "coefficient wise product"
      max_delta_sqr = fmax(max_delta_sqr, (v - v.cwiseProduct(scale)).squaredNorm());
    }
  }
  return max_delta_sqr;
}

size_t calc_num_slices(const LinearExtrudeNode& node, const Polygon2d& poly)
{
  size_t num_slices;
  if (node.has_slices) {
    return node.slices;
  }

  if (node.has_twist) {
    double max_r1_sqr = 0;  // r1 is before scaling
    for (const auto& o : poly.outlines())
      for (const auto& v : o.vertices) max_r1_sqr = fmax(max_r1_sqr, v.squaredNorm());
    if (node.scale_x == 1.0 && node.scale_y == 1.0) {
      // Calculate Helical curve length for Twist with no Scaling
      num_slices = (unsigned int)node.discretizer.getHelixSlices(max_r1_sqr, node.height[2], node.twist)
                     .value_or(std::max(static_cast<int>(std::ceil(node.twist / 120.0)), 1));
    } else if (node.scale_x != node.scale_y) {
      // Non-uniform scaling with twist.

      Vector2d scale(node.scale_x, node.scale_y);
      double max_delta_sqr = calc_max_delta_sqr(poly.outlines(), scale);

      // Why would we not find the furthest *scaled* max_r1_sqr and use getHelixSlices on that?
      // Because it scales non-uniformly and so you need a formula for the
      // length of a non-uniformly scaled helix.
      // And you would need to check every vertex, because the vertex that's furthest away
      // before you start twisting may not be the furthest throughout the twist.
      // Consider vertices at (3.99,2) and (4,-2), and a scale=(1,2).
      // If you rotate 90 degrees ACW, the first vertex will be further away,
      // but the second vertex starts and ends out further.

      size_t slicesNonUniScale =
        (unsigned int)node.discretizer.getDiagonalSlices(max_delta_sqr, node.height[2]).value_or(1);
      size_t slicesTwist =
        (unsigned int)node.discretizer.getHelixSlices(max_r1_sqr, node.height[2], node.twist)
          .value_or(std::max(static_cast<int>(std::ceil(node.twist / 120.0)), 1));
      num_slices = std::max(slicesNonUniScale, slicesTwist);
    } else {
      // uniform scaling with twist, use conical helix calculation
      num_slices = (unsigned int)node.discretizer
                     .getConicalHelixSlices(max_r1_sqr, node.height[2], node.twist, node.scale_x)
                     .value_or(std::max(static_cast<int>(std::ceil(node.twist / 120.0)), 1));
    }
  } else if (node.scale_x != node.scale_y) {
    // Non uniform scaling, w/o twist

    // Naively it doesn't seem like this case needs to be treated differently than uniform scaling,
    // because the line between the same 2d vertex is exactly straight.
    // But the faces will have error.
    // To see, animate this with fps=4 and steps=10:
    // module s(s) linear_extrude(h=30, scale=[1/20,20], slices=s) scale([1,0.1]) rotate(45) square(10,
    // center=true); steps=10; linear_extrude(h=1) projection(cut=true)
    // {
    //   low_slices=($t*steps)+1;
    //   translate([0,0,low_slices%2==1? -15 : -15+30/low_slices/2]) union() {
    //     difference() { s(low_slices); s(40);}
    //     difference() { s(40); s(low_slices);}
    //   }
    // }

    double max_delta_sqr = calc_max_delta_sqr(poly.outlines(), Vector2d(node.scale_x, node.scale_y));
    num_slices = node.discretizer.getDiagonalSlices(max_delta_sqr, node.height[2]).value_or(1);
  } else {
    // uniform scaling w/o twist needs only one slice
    num_slices = 1;
  }
  return num_slices;
}

/**
 * Break out extrusion logic to be used in unit tests.
 * @param slice_stride return value
 * @param vertices return value
 * @param indices return value
 */
void prepareVerticesAndIndices(const Polygon2d& polyref, Vector3d h1, Vector3d h2, int num_slices,
                               double scale_x, double scale_y, double twist,
                               std::vector<Vector3d>& vertices, PolygonIndices& indices,
                               int& slice_stride)
{
  for (const auto& o : polyref.outlines()) {
    slice_stride += o.vertices.size();
  }
  vertices.reserve(slice_stride * (num_slices + 1));
  indices.reserve(slice_stride * (num_slices + 1) * 2);  // sides + endcaps

  // Calculate all vertices
  Vector2d full_scale(1 - scale_x, 1 - scale_y);
  double full_rot = -twist;
  auto full_height = (h2 - h1);
  for (unsigned int slice_idx = 0; slice_idx <= num_slices; slice_idx++) {
    Eigen::Affine2d trans(Eigen::Scaling(Vector2d(1, 1) - full_scale * slice_idx / num_slices) *
                          Eigen::Affine2d(rotate_degrees(full_rot * slice_idx / num_slices)));

    for (const auto& o : polyref.outlines()) {
      for (const auto& v : o.vertices) {
        auto tmp = trans * v;
        vertices.emplace_back(Vector3d(tmp[0], tmp[1], 0.0) + h1 + full_height * slice_idx / num_slices);
      }
    }
  }

  // Create indices for sides
  for (unsigned int slice_idx = 1; slice_idx <= num_slices; slice_idx++) {
    double rot_prev = twist * (slice_idx - 1) / num_slices;
    double rot_curr = twist * slice_idx / num_slices;
    Vector2d scale_prev(1 - (1 - scale_x) * (slice_idx - 1) / num_slices,
                        1 - (1 - scale_y) * (slice_idx - 1) / num_slices);
    Vector2d scale_curr(1 - (1 - scale_x) * slice_idx / num_slices,
                        1 - (1 - scale_y) * slice_idx / num_slices);
    add_slice_indices(indices, slice_idx, slice_stride, polyref, rot_prev, rot_curr, scale_prev,
                      scale_curr);
  }
}

}  // namespace LinearExtrudeInternals

using namespace LinearExtrudeInternals;

/*!
   Input to extrude should be sanitized. This means non-intersecting, correct winding order
   etc., the input coming from a library like Clipper.
 */
std::unique_ptr<Geometry> extrudePolygon(const LinearExtrudeNode& node, const Polygon2d& poly)
{
  assert(poly.isSanitized());
  if (node.height[2] <= 0) return PolySet::createEmpty();

  bool non_linear = node.twist != 0 || node.scale_x != node.scale_y;
  boost::tribool isConvex{poly.is_convex()};
  // Twist makes convex polygons into unknown polyhedrons
  if (isConvex && non_linear) isConvex = unknown;

  // num_slices is the number of volumetric segments, minimum 1.
  // The number of rings of vertices will be num_slices+1.
  auto num_slices = calc_num_slices(node, poly);

  // Calculate outline segments if appropriate.
  Polygon2d seg_poly;

  // We skip segmentation if the user passed in segments=0
  if (!(node.has_segments && node.segments == 0)) {
    // A straight evenly-scaled linear extrusion doesn't lose detail, so we
    // only segment if the user requests it or we're doing something funky.
    if (node.segments > 0 || non_linear) {
      for (const auto& o : poly.outlines()) {
        seg_poly.addOutline(node.discretizer.splitOutline(o, node.twist, node.scale_x, node.scale_y,
                                                          num_slices, node.segments));
      }
    }
  }

  const Polygon2d& polyref = seg_poly.isEmpty() ? poly : seg_poly;

  Vector3d h1 = Vector3d::Zero();
  Vector3d h2 = node.height;

  if (node.center) {
    h1 -= node.height / 2.0;
    h2 -= node.height / 2.0;
  }

  int slice_stride = 0;
  std::vector<Vector3d> vertices;
  PolygonIndices indices;
  prepareVerticesAndIndices(polyref, h1, h2, num_slices, node.scale_x, node.scale_y, node.twist,
                            vertices, indices, slice_stride);

  // For Manifold, we can tesselate the endcaps using existing vertices to build a manifold mesh.
  // Without Manifold, however, we don't have such a tessellator available, so we'll have to build
  // the polyset from vertices using PolySetBuilder

#ifdef ENABLE_MANIFOLD
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
    return assemblePolySetForManifold(polyref, std::move(vertices), std::move(indices), node.convexity,
                                      isConvex, slice_stride * num_slices);
  } else
#endif
    return assemblePolySetForCGAL(polyref, vertices, indices, node.convexity, isConvex, node.scale_x,
                                  node.scale_y, h1, h2, node.twist);
}

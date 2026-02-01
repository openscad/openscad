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
#include "geometry/Barcode1d.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "utils/degree_trig.h"

namespace LinearExtrudeInternals {

/*
  Compare Euclidean length of vectors
  Return:
   -1 : if v1  < v2
    0 : if v1 ~= v2 (approximation to compensate for floating point precision)
    1 : if v1  > v2
*/
int sgn_vdiff(const Vector2d& v1, const Vector2d& v2)
{
  constexpr double ratio_threshold = 1e5;  // 5 orders-of-magnitude difference
  double l1 = v1.norm();
  double l2 = v2.norm();
  // Compare the average and difference, to be independent of geometry scale.
  // If the difference is within ratio_threshold of the avg, treat as equal.
  double scale = (l1 + l2);
  double diff = 2 * std::fabs(l1 - l2) * ratio_threshold;
  return diff > scale ? (l1 < l2 ? -1 : 1) : 0;
}

/**
 *
 * @param vertices The first polyref.length() vertices must be in the same order as polyref and represent
 * the bottom face. Similarly, the last polyref.length() vertices must be in the same order as polyref
 * and represent the top face.
 * @param indices These indexed face sets must not include the top nor bottom faces.
 */
std::unique_ptr<PolySet> assemblePolySetForManifold(const Polygon2d& polyref,
                                                    std::vector<Vector3d>& vertices,
                                                    PolygonIndices& indices,
                                                    std::vector<Color4f>& colors,
                                                    std::vector<int>& color_indices, int convexity,
                                                    boost::tribool isConvex, int index_offset)
{
  auto final_polyset = std::make_unique<PolySet>(3, isConvex);
  final_polyset->setTriangular(true);
  final_polyset->setConvexity(convexity);
  final_polyset->vertices = std::move(vertices);
  final_polyset->indices = std::move(indices);
  final_polyset->colors = std::move(colors);
  final_polyset->color_indices = std::move(color_indices);

  std::vector<int> colormap;
  for (int i = 0; i < final_polyset->vertices.size(); i++) colormap.push_back(0);
  for (int i = 0; i < final_polyset->indices.size(); i++) {
    auto& pol = final_polyset->indices[i];
    for (auto ind : pol) colormap[ind] = final_polyset->color_indices[i];
  }
  // Create top and bottom face.
  auto ps_bottom = polyref.tessellate();  // bottom
  // Flip vertex ordering for bottom polygon
  for (auto& p : ps_bottom->indices) {
    std::reverse(p.begin(), p.end());
  }
  std::copy(ps_bottom->indices.begin(), ps_bottom->indices.end(),
            std::back_inserter(final_polyset->indices));

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

  for (int j = final_polyset->color_indices.size(); j < final_polyset->indices.size(); j++) {
    final_polyset->color_indices.push_back(colormap[final_polyset->indices[j][0]]);
  }
  return final_polyset;
}

/*
   Attempt to triangulate quads in an ideal way.
   Each quad is composed of two adjacent outline vertices: (prev1, curr1)
   and their corresponding transformed points one step up: (prev2, curr2).
   Quads are triangulated across the shorter of the two diagonals, which works well in most cases.
   However, when diagonals are equal length, decision may flip depending on other factors.
 */
void add_slice_indices(PolygonIndices& indices, std::vector<Color4f>& colors,
                       std::vector<int>& color_indices, int slice_idx, int slice_stride,
                       const Polygon2d& poly, double rot1, double rot2, const Vector2d& scale1,
                       const Vector2d& scale2)
{
  int bottom_offset = (slice_idx - 1) * slice_stride;
  int top_offset = slice_idx * slice_stride;

  Eigen::Affine2d trans_bot(Eigen::Scaling(scale_slice_bottom) *
                            Eigen::Affine2d(rotate_degrees(-rotation_slice_bottom)));
  Eigen::Affine2d trans_top(Eigen::Scaling(scale_slice_top) *
                            Eigen::Affine2d(rotate_degrees(-rotation_slice_top)));

  bool any_zero = scale_slice_top[0] == 0 || scale_slice_top[1] == 0;
  // setting back_twist true helps keep diagonals same as previous builds.
  bool back_twist = rotation_slice_top <= rotation_slice_bottom;

  int curr_outline = 0;
  for (const auto& o : poly.outlines()) {
    // prev_vtx_bot: previous vertex, on the bottom of this slice
    // prev_vtx_top: previous vertex, on the top of this slice
    Vector2d prev_vtx_bot = trans_bot * o.vertices[0];
    Vector2d prev_vtx_top = trans_top * o.vertices[0];

    // For equal length diagonals, flip selected choice depending on direction of twist and
    // whether the outline is negative (eg circle hole inside a larger circle).
    // This was tested against circles with a single point touching the origin,
    // and extruded with twist.  Diagonal choice determined by whichever option
    // matched the direction of diagonal for neighboring edges (which did not exhibit "equal" diagonals).
    bool flip = ((!o.positive) xor (back_twist));
    int color_ind = colors.size();
    colors.push_back(o.color);
    for (int i = 1; i <= o.vertices.size(); ++i) {
      // curr1: previous slice, current vertex
      // curr2: current slice, current vertex
      Vector2d curr1 = trans1 * o.vertices[i % o.vertices.size()];
      Vector2d curr2 = trans2 * o.vertices[i % o.vertices.size()];
      int curr_idx = curr_outline + (i % o.vertices.size());
      int prev_idx = curr_outline + i - 1;

      // -1 if diagonal from current-slice-current-vertex to previous-slice-previous-vertex is smaller
      int diff_sign = sgn_vdiff(prev_vtx_bot - vtx_top, vtx_bot - prev_vtx_top);
      bool splitfirst = diff_sign == -1 || (diff_sign == 0 && !flip);

      // Split along shortest diagonal,
      // unless at top for a 0-scaled axis (which can create 0 thickness "ears")
      if (splitfirst xor any_zero) {
        indices.push_back({
          bottom_offset + idx,
          top_offset + idx,
          bottom_offset + prev_idx,
        });
        indices.push_back({
          top_offset + prev_idx,
          bottom_offset + prev_idx,
          top_offset + idx,
        });
      } else {
        indices.push_back({
          bottom_offset + idx,
          top_offset + prev_idx,
          bottom_offset + prev_idx,
        });
        indices.push_back({
          bottom_offset + idx,
          top_offset + idx,
          top_offset + prev_idx,
        });
      }
      color_indices.push_back(color_ind);
      color_indices.push_back(color_ind);
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
      num_slices = static_cast<unsigned int>(
        node.discretizer.getHelixSlices(max_r1_sqr, node.height[2], node.twist)
          .value_or(std::max(static_cast<int>(std::ceil(node.twist / 120.0)), 1)));
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
      // If you rotate 90 degrees CCW, the first vertex will be further away,
      // but the second vertex starts and ends out further.

      size_t slicesNonUniScale = static_cast<unsigned int>(
        node.discretizer.getDiagonalSlices(max_delta_sqr, node.height[2]).value_or(1));
      size_t slicesTwist = static_cast<unsigned int>(
        node.discretizer.getHelixSlices(max_r1_sqr, node.height[2], node.twist)
          .value_or(std::max(static_cast<int>(std::ceil(node.twist / 120.0)), 1)));
      num_slices = std::max(slicesNonUniScale, slicesTwist);
    } else {
      // uniform scaling with twist, use conical helix calculation
      num_slices = static_cast<unsigned int>(
        node.discretizer.getConicalHelixSlices(max_r1_sqr, node.height[2], node.twist, node.scale_x)
          .value_or(std::max(static_cast<int>(std::ceil(node.twist / 120.0)), 1)));
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
    double rot_bot = twist * (slice_idx - 1) / num_slices;
    double rot_top = twist * slice_idx / num_slices;
    Vector2d scale_bot(1 - (1 - scale_x) * (slice_idx - 1) / num_slices,
                       1 - (1 - scale_y) * (slice_idx - 1) / num_slices);
    Vector2d scale_top(1 - (1 - scale_x) * slice_idx / num_slices,
                       1 - (1 - scale_y) * slice_idx / num_slices);
    add_slice_indices(indices, slice_idx, slice_stride, polyref, rot_bot, rot_top, scale_bot, scale_top);
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
  if (fabs(node.height[2]) < 1e-6) return PolySet::createEmpty();

  bool non_linear = node.twist != 0 || node.scale_x != node.scale_y;
  boost::tribool isConvex{poly.is_convex()};
  // Twist makes convex polygons into unknown polyhedrons
  if (isConvex && non_linear) isConvex = unknown;

  // num_slices is the number of volumetric segments, minimum 1.
  // The number of rings of vertices will be num_slices+1.
  auto num_slices = calc_num_slices(node, poly);

  // Calculate outline segments if appropriate.
  Polygon2d seg_poly;
  seg_poly.transform3d(poly.getTransform3d());

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
#ifdef ENABLE_PYTHON
  if (node.profile_func != nullptr) {
    seg_poly = python_getprofile(node.profile_func, 3, 0);
  }
#endif

  Vector3d height(0, 0, 1);
  if (node.has_heightvector) height = node.height;
  else {
    auto mat = seg_poly.getTransform3d();
    height = Vector3d(mat(0, 2), mat(1, 2), mat(2, 2)).normalized() * node.height[2];
  }
  Polygon2d polyref = seg_poly.isEmpty() ? poly : seg_poly;
  if (node.height[2] < 0) {
    // reverse points, to not get a left system
    polyref.reverse();
  }

  Vector3d h1 = Vector3d::Zero();
  Vector3d h2 = height;

  if (node.center) {
    h1 -= height / 2.0;
    h2 -= height / 2.0;
  }

  int slice_stride = 0;
  std::vector<Vector3d> vertices;
  PolygonIndices indices;
  indices.reserve(slice_stride * (num_slices + 1) * 2);  // sides + endcaps

  // Calculate all vertices
  Vector2d full_scale(1 - node.scale_x, 1 - node.scale_y);
  double full_rot = -node.twist;
  auto full_height = (h2 - h1);
  for (unsigned int slice_idx = 0; slice_idx <= num_slices; slice_idx++) {
    double act_rot;
#ifdef ENABLE_PYTHON
    if (node.twist_func != NULL) {
      act_rot = python_doublefunc(node.twist_func, (double)(slice_idx / num_slices));
    } else
#endif
      act_rot = full_rot * slice_idx / num_slices;

    Eigen::Affine2d trans(Eigen::Scaling(Vector2d(1, 1) - full_scale * slice_idx / num_slices) *
                          Eigen::Affine2d(rotate_degrees(act_rot)));

#ifdef ENABLE_PYTHON
    if (node.profile_func == nullptr)
#endif
    {
      Transform3d tr = polyref.getTransform3d();
      for (const auto& o : polyref.untransformedOutlines()) {
        for (const auto& v : o.vertices) {
          auto tmp = trans * v;
          Vector3d tmp1 = tr * Vector3d(tmp[0], tmp[1], 0.0);
          vertices.emplace_back(tmp1 + h1 + full_height * slice_idx / num_slices);
        }
      }
    }
  }
  std::vector<Color4f> colors;
  std::vector<int> color_indices;
#ifdef ENABLE_PYTHON
  if (node.profile_func != nullptr) {
    // completely differet alg
    PolySetBuilder builder(0, 0, 3, isConvex);
    std::vector<Vector3d> botvertices;
    for (unsigned int slice_idx = 0; slice_idx <= num_slices; slice_idx++) {
      double act_rot;
      if (node.twist_func != NULL) {
        act_rot = python_doublefunc(node.twist_func, (double)(slice_idx / num_slices));
      } else act_rot = full_rot * slice_idx / num_slices;

      Eigen::Affine2d trans(Eigen::Scaling(Vector2d(1, 1) - full_scale * slice_idx / num_slices) *
                            Eigen::Affine2d(rotate_degrees(act_rot)));
      auto prof = python_getprofile(node.profile_func, 3, full_height[2] * slice_idx / num_slices);
      std::vector<Vector3d> topvertices;
      for (const auto& v : prof.vertices) {
        auto tmp = trans * v;
        topvertices.push_back(Vector3d(tmp[0], tmp[1], 0.0) + h1 + full_height * slice_idx / num_slices);
      }

      if (botvertices.size() > 0) {
        int nbot = botvertices.size();
        int ntop = topvertices.size();
        int ibot = 0;
        int itop = 0;
        int top_off = 0;
        double distmin = 0;
        for (int i = 0; i < ntop; i++) {
          double dist = (botvertices[ibot] - topvertices[i]).norm();
          if (i == 0 || dist < distmin) {
            distmin = dist;
            top_off = i;
          }
        }
        // calculate next step
        while (ibot < nbot || itop < ntop) {
          double dist_bot =
            (botvertices[(ibot + 1) % nbot] - topvertices[(itop + top_off) % ntop]).norm();
          double dist_top = (botvertices[ibot] - topvertices[(itop + top_off + 1) % ntop]).norm();
          if ((dist_bot < dist_top && ibot < nbot) || (itop == ntop)) {
            builder.beginPolygon(3);
            builder.addVertex(botvertices[ibot % nbot]);
            builder.addVertex(botvertices[(ibot + 1) % nbot]);
            builder.addVertex(topvertices[(itop + top_off) % ntop]);
            builder.endPolygon();
            ibot++;
          } else {  // distr_top < dist_bot or itop < ntop
            builder.beginPolygon(3);
            builder.addVertex(botvertices[ibot % nbot]);
            builder.addVertex(topvertices[(itop + top_off + 1) % ntop]);
            builder.addVertex(topvertices[(itop + top_off) % ntop]);
            builder.endPolygon();
            itop++;
          }
        }
      } else {
        builder.beginPolygon(topvertices.size());  // bottom
        for (int i = topvertices.size() - 1; i >= 0; i--) builder.addVertex(topvertices[i]);
        builder.endPolygon();
      }
      botvertices = topvertices;
    }
    builder.beginPolygon(botvertices.size());  // top
    for (size_t i = 0; i < botvertices.size(); i++) builder.addVertex(botvertices[i]);
    builder.endPolygon();

    return builder.build();
  }
#endif

  // Create indices for sides
  for (unsigned int slice_idx = 1; slice_idx <= num_slices; slice_idx++) {
    double rot_prev = node.twist * (slice_idx - 1) / num_slices;
    double rot_curr = node.twist * slice_idx / num_slices;
    Vector2d scale_prev(1 - (1 - node.scale_x) * (slice_idx - 1) / num_slices,
                        1 - (1 - node.scale_y) * (slice_idx - 1) / num_slices);
    Vector2d scale_curr(1 - (1 - node.scale_x) * slice_idx / num_slices,
                        1 - (1 - node.scale_y) * slice_idx / num_slices);
    add_slice_indices(indices, colors, color_indices, slice_idx, slice_stride, polyref, rot_prev,
                      rot_curr, scale_prev, scale_curr);
  }

  // For Manifold, we can tesselate the endcaps using existing vertices to build a manifold mesh.
  // Without Manifold, however, we don't have such a tessellator available, so we'll have to build
  // the polyset from vertices using PolySetBuilder

#ifdef ENABLE_MANIFOLD
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
    return assemblePolySetForManifold(polyref, vertices, indices, colors, color_indices, node.convexity,
                                      isConvex, slice_stride * num_slices);
  } else
#endif
    return assemblePolySetForManifold(polyref, vertices, indices, colors, color_indices, node.convexity,
                                      isConvex, slice_stride * num_slices);
}

std::unique_ptr<Geometry> extrudeBarcode(const LinearExtrudeNode& node, const Barcode1d& barcode)
{
  Polygon2d p;
  for (auto e : barcode.untransformedEdges()) {
    Vector2d v1(e.begin, 0);
    Vector2d v2(e.begin, node.height[2]);
    Vector2d v3(e.end, node.height[2]);
    Vector2d v4(e.end, 0);

    Outline2d o;
    o.color = e.color;
    o.vertices = {v1, v2, v3, v4};
    p.addOutline(o);
  }
  p.transform3d(barcode.getTransform3d());
  p.setSanitized(true);
  return std::make_unique<Polygon2d>(p);
}

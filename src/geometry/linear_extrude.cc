#include "geometry/linear_extrude.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <cassert>
#include <utility>
#include <memory>
#include <cstddef>
#include <queue>
#include <vector>

#include <boost/logic/tribool.hpp>

#include "geometry/GeometryUtils.h"
#include "glview/RenderSettings.h"
#include "core/LinearExtrudeNode.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "utils/calc.h"
#include "utils/degree_trig.h"

namespace {

/*
  Compare Euclidean length of vectors
  Return:
   -1 : if v1  < v2
    0 : if v1 ~= v2 (approximation to compoensate for floating point precision)
    1 : if v1  > v2
*/
int sgn_vdiff(const Vector2d& v1, const Vector2d& v2) {
  constexpr double ratio_threshold = 1e5; // 10ppm difference
  double l1 = v1.norm();
  double l2 = v2.norm();
  // Compare the average and difference, to be independent of geometry scale.
  // If the difference is within ratio_threshold of the avg, treat as equal.
  double scale = (l1 + l2);
  double diff = 2 * std::fabs(l1 - l2) * ratio_threshold;
  return diff > scale ? (l1 < l2 ? -1 : 1) : 0;
}

// Insert vertices for segments interpolated between v0 and v1.
// The last vertex (t==1) is not added here to avoid duplicate vertices,
// since it will be the first vertex of the *next* edge.
void add_segmented_edge(Outline2d& o, const Vector2d& v0, const Vector2d& v1, unsigned int edge_segments) {
  for (unsigned int j = 0; j < edge_segments; ++j) {
    double t = static_cast<double>(j) / edge_segments;
    o.vertices.push_back((1 - t) * v0 + t * v1);
  }
}

// While total outline segments < fn, increment segment_count for edge with largest
// (max_edge_length / segment_count).
Outline2d splitOutlineByFn(
  const Outline2d& o,
  const double twist, const double scale_x, const double scale_y,
  const double fn, unsigned int slices)
{

  struct segment_tracker {
    size_t edge_index;
    double max_edgelen;
    unsigned int segment_count{1u};
    segment_tracker(size_t i, double len) : edge_index(i), max_edgelen(len) { }
    // metric for comparison: average between (max segment length, and max segment length after split)
    [[nodiscard]] double metric() const { return max_edgelen / (segment_count + 0.5); }
    bool operator<(const segment_tracker& rhs) const { return this->metric() < rhs.metric();  }
    [[nodiscard]] bool close_match(const segment_tracker& other) const {
      // Edges are grouped when metrics match by at least 99.9%
      constexpr double APPROX_EQ_RATIO = 0.999;
      double l1 = this->metric(), l2 = other.metric();
      return std::min(l1, l2) / std::max(l1, l2) >= APPROX_EQ_RATIO;
    }
  };

  const auto num_vertices = o.vertices.size();
  std::vector<unsigned int> segment_counts(num_vertices, 1);
  std::priority_queue<segment_tracker, std::vector<segment_tracker>> q;

  Vector2d v0 = o.vertices[0];
  // non-uniform scaling requires iterating over each slice transform
  // to find maximum length of a given edge.
  if (scale_x != scale_y) {
    for (size_t i = 1; i <= num_vertices; ++i) {
      Vector2d v1 = o.vertices[i % num_vertices];
      double max_edgelen = 0.0; // max length for single edge over all transformed slices
      for (unsigned int j = 0; j <= slices; j++) {
        double t = static_cast<double>(j) / slices;
        Vector2d scale(Calc::lerp(1, scale_x, t), Calc::lerp(1, scale_y, t));
        double rot = twist * t;
        Eigen::Affine2d trans(Eigen::Scaling(scale) * Eigen::Affine2d(rotate_degrees(-rot)));
        double edgelen = (trans * v1 - trans * v0).norm();
        max_edgelen = std::max(max_edgelen, edgelen);
      }
      q.emplace(i - 1, max_edgelen);
      v0 = v1;
    }
  } else { // uniform scaling
    double max_scale = std::max(scale_x, 1.0);
    for (size_t i = 1; i <= num_vertices; ++i) {
      Vector2d v1 = o.vertices[i % num_vertices];
      double max_edgelen = (v1 - v0).norm() * max_scale;
      q.emplace(i - 1, max_edgelen);
      v0 = v1;
    }
  }

  std::vector<segment_tracker> tmp_q;
  // Process priority_queue until number of segments is reached.
  size_t seg_total = num_vertices;
  while (seg_total < fn) {
    auto current = q.top();

    // Group similar length segmented edges to keep result roughly symmetrical.
    while (!q.empty() && (tmp_q.empty() || q.top().close_match(tmp_q.front()))) {
      tmp_q.push_back(q.top());
      q.pop();
    }

    if (seg_total + tmp_q.size() <= fn) {
      while (!tmp_q.empty()) {
        current = tmp_q.back();
        tmp_q.pop_back();
        ++current.segment_count;
        ++segment_counts[current.edge_index];
        ++seg_total;
        q.push(current);
      }
    } else {
      // fn too low to segment last group, push back onto queue without change.
      while (!tmp_q.empty()) {
        current = tmp_q.back();
        tmp_q.pop_back();
        q.push(current);
      }
      break;
    }
  }

  // Create final segmented edges.
  Outline2d o2;
  o2.positive = o.positive;
  v0 = o.vertices[0];
  for (size_t i = 1; i <= num_vertices; ++i) {
    Vector2d v1 = o.vertices[i % num_vertices];
    add_segmented_edge(o2, v0, v1, segment_counts[i - 1]);
    v0 = v1;
  }

  assert(o2.vertices.size() <= fn);
  return o2;
}

// For each edge in original outline, find its max length over all slice transforms,
// and divide into segments no longer than fs.
Outline2d splitOutlineByFs(
  const Outline2d& o,
  const double twist, const double scale_x, const double scale_y,
  const double fs, unsigned int slices)
{
  const auto num_vertices = o.vertices.size();

  Vector2d v0 = o.vertices[0];
  Outline2d o2;
  o2.positive = o.positive;

  // non-uniform scaling requires iterating over each slice transform
  // to find maximum length of a given edge.
  if (scale_x != scale_y) {
    for (size_t i = 1; i <= num_vertices; ++i) {
      Vector2d v1 = o.vertices[i % num_vertices];
      double max_edgelen = 0.0; // max length for single edge over all transformed slices
      for (unsigned int j = 0; j <= slices; j++) {
        double t = static_cast<double>(j) / slices;
        Vector2d scale(Calc::lerp(1, scale_x, t), Calc::lerp(1, scale_y, t));
        double rot = twist * t;
        Eigen::Affine2d trans(Eigen::Scaling(scale) * Eigen::Affine2d(rotate_degrees(-rot)));
        double edgelen = (trans * v1 - trans * v0).norm();
        max_edgelen = std::max(max_edgelen, edgelen);
      }
      auto edge_segments = static_cast<unsigned int>(std::ceil(max_edgelen / fs));
      add_segmented_edge(o2, v0, v1, edge_segments);
      v0 = v1;
    }
  } else { // uniform scaling
    double max_scale = std::max(scale_x, 1.0);
    for (size_t i = 1; i <= num_vertices; ++i) {
      Vector2d v1 = o.vertices[i % num_vertices];
      unsigned int edge_segments = static_cast<unsigned int>(std::ceil((v1 - v0).norm() * max_scale / fs));
      add_segmented_edge(o2, v0, v1, edge_segments);
      v0 = v1;
    }
  }
  return o2;
}

std::unique_ptr<PolySet> assemblePolySetForManifold(
  const Polygon2d& polyref,
  std::vector<Vector3d>& vertices, PolygonIndices& indices,
  int convexity, boost::tribool isConvex, int index_offset) {
  auto final_polyset = std::make_unique<PolySet>(3, isConvex);
  final_polyset->setTriangular(true);
  final_polyset->setConvexity(convexity);
  final_polyset->vertices = std::move(vertices);
  final_polyset->indices = std::move(indices);

  // Create top and bottom face.
  auto ps_bottom = polyref.tessellate(); // bottom
  // Flip vertex ordering for bottom polygon
  for (auto& p : ps_bottom->indices) {
    std::reverse(p.begin(), p.end());
  }
  std::copy(ps_bottom->indices.begin(), ps_bottom->indices.end(),
     std::back_inserter(final_polyset->indices));

  for (auto& p : ps_bottom->indices) {
    std::reverse(p.begin(), p.end());
    for (auto& i : p) {
      i += index_offset;
    }
  }
  std::copy(ps_bottom->indices.begin(), ps_bottom->indices.end(),
     std::back_inserter(final_polyset->indices));

  // LOG(PolySetUtils::polySetToPolyhedronSource(*final_polyset));

  return final_polyset;
}

std::unique_ptr<PolySet> assemblePolySetForCGAL(const Polygon2d& polyref,
  std::vector<Vector3d>& vertices, PolygonIndices& indices,
  int convexity, boost::tribool isConvex,
  double scale_x, double scale_y,
  const Vector3d& h1, const Vector3d& h2, double twist) {

  PolySetBuilder builder(0, 0, 3, isConvex);
  builder.setConvexity(convexity);

  for (const auto& poly: indices) {
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
  auto ps_bottom = polyref.tessellate(); // bottom
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
void add_slice_indices(PolygonIndices &indices, int slice_idx, int slice_stride, const Polygon2d& poly,
                              double rot1, double rot2,
                              const Vector2d& scale1, const Vector2d& scale2)
{
  int prev_slice = (slice_idx-1)*slice_stride;
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

    for (int i = 1; i <= o.vertices.size(); ++i) {
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

size_t calc_num_slices(const LinearExtrudeNode& node, const Polygon2d& poly) {
  size_t num_slices;
  if (node.has_slices) {
    num_slices = node.slices;
  } else if (node.has_twist) {
    double max_r1_sqr = 0; // r1 is before scaling
    Vector2d scale(node.scale_x, node.scale_y);
    for (const auto& o : poly.outlines())
      for (const auto& v : o.vertices)
        max_r1_sqr = fmax(max_r1_sqr, v.squaredNorm());
    // Calculate Helical curve length for Twist with no Scaling
    if (node.scale_x == 1.0 && node.scale_y == 1.0) {
      num_slices = (unsigned int)Calc::get_helix_slices(max_r1_sqr, node.height[2], node.twist, node.fn, node.fs, node.fa);
    } else if (node.scale_x != node.scale_y) {  // non uniform scaling with twist using max slices from twist and non uniform scale
      double max_delta_sqr = 0; // delta from before/after scaling
      Vector2d scale(node.scale_x, node.scale_y);
      for (const auto& o : poly.outlines()) {
        for (const auto& v : o.vertices) {
          max_delta_sqr = fmax(max_delta_sqr, (v - v.cwiseProduct(scale)).squaredNorm());
        }
      }
      size_t slicesNonUniScale;
      size_t slicesTwist;
      slicesNonUniScale = (unsigned int)Calc::get_diagonal_slices(max_delta_sqr, node.height[2], node.fn, node.fs);
      slicesTwist = (unsigned int)Calc::get_helix_slices(max_r1_sqr, node.height[2], node.twist, node.fn, node.fs, node.fa);
      num_slices = std::max(slicesNonUniScale, slicesTwist);
    } else { // uniform scaling with twist, use conical helix calculation
      num_slices = (unsigned int)Calc::get_conical_helix_slices(max_r1_sqr, node.height[2], node.twist, node.scale_x, node.fn, node.fs, node.fa);
    }
  } else if (node.scale_x != node.scale_y) {
    // Non uniform scaling, w/o twist
    double max_delta_sqr = 0; // delta from before/after scaling
    Vector2d scale(node.scale_x, node.scale_y);
    for (const auto& o : poly.outlines()) {
      for (const auto& v : o.vertices) {
        max_delta_sqr = fmax(max_delta_sqr, (v - v.cwiseProduct(scale)).squaredNorm());
      }
    }
    num_slices = Calc::get_diagonal_slices(max_delta_sqr, node.height[2], node.fn, node.fs);
  } else {
    // uniform or [1,1] scaling w/o twist needs only one slice
    num_slices = 1;
  }
  return num_slices;
}

}  // namespace

/*!
   Input to extrude should be sanitized. This means non-intersecting, correct winding order
   etc., the input coming from a library like Clipper.
 */
std::unique_ptr<Geometry> extrudePolygon(const LinearExtrudeNode& node, const Polygon2d& poly)
{
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
  bool is_segmented = false;
  if (node.has_segments) {
    // Set segments = 0 to disable
    if (node.segments > 0) {
      for (const auto& o : poly.outlines()) {
        if (o.vertices.size() >= node.segments) {
          seg_poly.addOutline(o);
        } else {
          seg_poly.addOutline(splitOutlineByFn(o, node.twist, node.scale_x, node.scale_y, node.segments, num_slices));
        }
      }
      is_segmented = true;
    }
  } else if (non_linear) {
    if (node.fn > 0.0) {
      for (const auto& o : poly.outlines()) {
        if (o.vertices.size() >= node.fn) {
          seg_poly.addOutline(o);
        } else {
          seg_poly.addOutline(splitOutlineByFn(o, node.twist, node.scale_x, node.scale_y, node.fn, num_slices));
        }
      }
    } else { // $fs and $fa based segmentation
      auto fa_segs = static_cast<unsigned int>(std::ceil(360.0 / node.fa));
      for (const auto& o : poly.outlines()) {
        if (o.vertices.size() >= fa_segs) {
          seg_poly.addOutline(o);
        } else {
          // try splitting by $fs, then check if $fa results in less segments
          auto fsOutline = splitOutlineByFs(o, node.twist, node.scale_x, node.scale_y, node.fs, num_slices);
          if (fsOutline.vertices.size() >= fa_segs) {
            seg_poly.addOutline(splitOutlineByFn(o, node.twist, node.scale_x, node.scale_y, fa_segs, num_slices));
          } else {
            seg_poly.addOutline(std::move(fsOutline));
          }
        }
      }
    }
    is_segmented = true;
  }

  const Polygon2d& polyref = is_segmented ? seg_poly : poly;

  Vector3d h1 = Vector3d::Zero();
  Vector3d h2 = node.height;

  if (node.center) {
    h1 -= node.height / 2.0;
    h2 -= node.height / 2.0;
  }

  int slice_stride = 0;
  for (const auto& o : polyref.outlines()) {
    slice_stride += o.vertices.size();
  }
  std::vector<Vector3d> vertices;
  vertices.reserve(slice_stride * (num_slices + 1));
  PolygonIndices indices;
  indices.reserve(slice_stride * (num_slices + 1) * 2); // sides + endcaps

  // Calculate all vertices
  Vector2d full_scale(1 - node.scale_x, 1 - node.scale_y);
  double full_rot = -node.twist;
  auto full_height = (h2 - h1);
  for (unsigned int slice_idx = 0; slice_idx <= num_slices; slice_idx++) {
    Eigen::Affine2d trans(
      Eigen::Scaling(Vector2d(1,1) - full_scale * slice_idx / num_slices) *
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
    double rot_prev = node.twist * (slice_idx -1)/ num_slices;
    double rot_curr = node.twist * slice_idx / num_slices;
    auto height_prev = h1 + (h2 - h1) * (slice_idx - 1) / num_slices;
    auto height_curr = h1 + (h2 - h1) * slice_idx / num_slices;
    Vector2d scale_prev(1 - (1 - node.scale_x) * (slice_idx - 1) / num_slices,
                    1 - (1 - node.scale_y) * (slice_idx - 1) / num_slices);
    Vector2d scale_curr(1 - (1 - node.scale_x) * slice_idx / num_slices,
                    1 - (1 - node.scale_y) * slice_idx / num_slices);
    add_slice_indices(indices, slice_idx, slice_stride, polyref, rot_prev, rot_curr, scale_prev, scale_curr);
  }

  // For Manifold, we can tesselate the endcaps using existing vertices to build a manifold mesh.
  // Without Manifold, however, we don't have such a tessellator available, so we'll have to build
  // the polyset from vertices using PolySetBuilder

#ifdef ENABLE_MANIFOLD
  if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend) {
    return assemblePolySetForManifold(polyref, vertices, indices,
                                      node.convexity, isConvex, slice_stride * num_slices);
  }
  else
#endif
  return assemblePolySetForCGAL(polyref, vertices, indices,
                                node.convexity, isConvex,
                                node.scale_x, node.scale_y,
                                h1, h2, node.twist);
}

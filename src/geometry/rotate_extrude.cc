#include "rotate_extrude.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "core/RotateExtrudeNode.h"
#include "core/CurveDiscretizer.h"
#include "geometry/GeometryUtils.h"
#include "geometry/Geometry.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "geometry/Polygon2d.h"
#include "geometry/linalg.h"
#include "geometry/PolySet.h"
#include "utils/calc.h"
#include "utils/degree_trig.h"
#include "utils/printutils.h"

static std::unique_ptr<PolySet> assemblePolySetForManifold(const Polygon2d& polyref,
                                                           std::vector<Vector3d>& vertices,
                                                           PolygonIndices& indices, bool closed,
                                                           int convexity, int index_offset,
                                                           bool flip_faces)
{
  auto final_polyset = std::make_unique<PolySet>(3, false);
  final_polyset->setTriangular(true);
  final_polyset->setConvexity(convexity);
  final_polyset->vertices = std::move(vertices);
  final_polyset->indices = std::move(indices);

  if (!closed) {
    // Create top and bottom face.
    auto ps_bottom = polyref.tessellate();  // bottom
    // Flip vertex ordering for bottom polygon unless flip_faces is true
    if (!flip_faces) {
      for (auto& p : ps_bottom->indices) {
        std::reverse(p.begin(), p.end());
      }
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
  }

  //  LOG(PolySetUtils::polySetToPolyhedronSource(*final_polyset));

  return final_polyset;
}

/*!
   Input to extrude should be clean. This means non-intersecting, correct winding order
   etc., the input coming from a library like Clipper.

   FIXME: We should handle some common corner cases better:
   o 2D polygon having an edge being on the Y axis:
    In this case, we don't need to generate geometry involving this edge as it
    will be an internal edge.
   o 2D polygon having a vertex touching the Y axis:
    This is more complex as the resulting geometry will (may?) be nonmanifold.
    In any case, the previous case is a specialization of this, so the following
    should be handled for both cases:
    Since the ring associated with this vertex will have a radius of zero, it will
    collapse to one vertex. Any quad using this ring will be collapsed to a triangle.

   Currently, we generate a lot of zero-area triangles
 */
std::unique_ptr<Geometry> rotatePolygon(const RotateExtrudeNode& node, const Polygon2d& poly)
{
  if (node.angle == 0) return nullptr;

  double min_x = 0;
  double max_x = 0;
  for (const auto& o : poly.outlines()) {
    for (const auto& v : o.vertices) {
      min_x = fmin(min_x, v[0]);
      max_x = fmax(max_x, v[0]);
    }
  }

  if (max_x > 0 && min_x < 0) {
    LOG(message_group::Error,
        "Children of rotate_extrude() may not lie across the Y axis (Range of X coords for all children "
        "[%1$.2f : %2$.2f])",
        min_x, max_x);
    return nullptr;
  }

  // # of sections. For closed rotations, # vertices is thus fragments*outline_size. For open
  // rotations # vertices is (fragments+1)*outline_size.
  const int num_sections = node.discretizer.getCircularSegmentCount(max_x - min_x, node.angle)
                             .value_or(std::max(1, static_cast<int>(std::fabs(node.angle) / 360 * 3)));
  const bool closed = node.angle == 360;
  // # of rings of vertices
  const size_t num_rings = num_sections + (closed ? 0 : 1);

  const bool flip_faces = (min_x >= 0 && node.angle > 0) || (min_x < 0 && node.angle < 0);

  // slice_stride is the number of vertices in a single ring
  size_t slice_stride = 0;
  for (const auto& o : poly.outlines()) {
    slice_stride += o.vertices.size();
  }
  const int num_vertices = slice_stride * num_rings;
  std::vector<Vector3d> vertices;
  vertices.reserve(num_vertices);
  PolygonIndices indices;
  indices.reserve(slice_stride * num_rings * 2);  // sides + endcaps if needed

  // Calculate all vertices
  for (unsigned int j = 0; j < num_rings; ++j) {
    for (const auto& outline : poly.outlines()) {
      const double angle = node.start + j * node.angle / num_sections;  // start on the X axis
      for (const auto& v : outline.vertices) {
        vertices.emplace_back(v[0] * cos_degrees(angle), v[0] * sin_degrees(angle), v[1]);
      }
    }
  }

  // Calculate all indices
  for (unsigned int slice_idx = 1; slice_idx <= num_sections; slice_idx++) {
    const int prev_slice = (slice_idx - 1) * slice_stride;
    const int curr_slice = slice_idx * slice_stride;
    int curr_outline = 0;
    for (const auto& outline : poly.outlines()) {
      assert(outline.vertices.size() > 2);
      for (size_t i = 1; i <= outline.vertices.size(); ++i) {
        const int curr_idx = curr_outline + (i % outline.vertices.size());
        const int prev_idx = curr_outline + i - 1;
        if (flip_faces) {
          indices.push_back({
            (prev_slice + prev_idx) % num_vertices,
            (curr_slice + curr_idx) % num_vertices,
            (prev_slice + curr_idx) % num_vertices,
          });
          indices.push_back({
            (curr_slice + curr_idx) % num_vertices,
            (prev_slice + prev_idx) % num_vertices,
            (curr_slice + prev_idx) % num_vertices,
          });
        } else {
          indices.push_back({
            (prev_slice + curr_idx) % num_vertices,
            (curr_slice + curr_idx) % num_vertices,
            (prev_slice + prev_idx) % num_vertices,
          });
          indices.push_back({
            (curr_slice + prev_idx) % num_vertices,
            (prev_slice + prev_idx) % num_vertices,
            (curr_slice + curr_idx) % num_vertices,
          });
        }
      }
      curr_outline += outline.vertices.size();
    }
  }

  // TODO(kintel): Without Manifold, we don't have such tessellator available which guarantees to not
  // modify vertices, so we technically may end up with broken end caps if we build OpenSCAD without
  // ENABLE_MANIFOLD. Should be fixed, but it's low priority and it's not trivial to come up with a test
  // case for this.
  return assemblePolySetForManifold(poly, vertices, indices, closed, node.convexity,
                                    slice_stride * num_sections, flip_faces);
}

#include "rotate_extrude.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <iterator>
#include <memory>
#include <vector>

#include "core/RotateExtrudeNode.h"
#include "geometry/Geometry.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/PolySetUtils.h"
#include "geometry/Polygon2d.h"
#include "geometry/linalg.h"
#include "geometry/PolySet.h"
#include "utils/calc.h"
#include "utils/degree_trig.h"
#include "utils/printutils.h"

namespace {

template <class Iter>
void fill_ring(Iter ring, const Outline2d& outline, double angle)
{
  for (const auto& v : outline.vertices) {
    ring = Vector3d(v[0] * cos_degrees(angle), v[0] * sin_degrees(angle), v[1]);
  }
}

}  // namespace

std::unique_ptr<PolySet> assemblePolySetForManifold(const Polygon2d& polyref,
                                                    std::vector<Vector3d>& vertices,
                                                    PolygonIndices& indices, bool closed, int convexity,
                                                    int index_offset, bool flip_faces)
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

  //  PolySetBuilder builder;
  //  builder.setConvexity(node.convexity);

  double min_x = 0;
  double max_x = 0;
  for (const auto& o : poly.outlines()) {
    for (const auto& v : o.vertices) {
      min_x = fmin(min_x, v[0]);
      max_x = fmax(max_x, v[0]);
    }
  }

  if (max_x > 0 && min_x < 0) {
    LOG(
      message_group::Error,
      "all points for rotate_extrude() must have the same X coordinate sign (range is %1$.2f -> %2$.2f)",
      min_x, max_x);
    return nullptr;
  }

  // # of sections. For closed rotations, # vertices is thus fragments*outline_size. For open
  // rotations # vertices is (fragments+1)*outline_size.
  const auto num_sections = (unsigned int)std::ceil(fmax(
    Calc::get_fragments_from_r(max_x - min_x, node.fn, node.fs, node.fa) * std::abs(node.angle) / 360,
    1));
  const bool closed = node.angle == 360;
  // # of rings of vertices
  const int num_rings = num_sections + (closed ? 0 : 1);

  const bool flip_faces = (min_x >= 0 && node.angle > 0) || (min_x < 0 && node.angle < 0);

  //  // If not going all the way around, we have to create faces on each end.
  //  if (!closed) {
  //    const auto ps_start = poly.tessellate(); // starting face
  //    const Transform3d rotx(angle_axis_degrees(90, Vector3d::UnitX()));
  //    const Transform3d rotz1(angle_axis_degrees(node.start, Vector3d::UnitZ()));
  //    ps_start->transform(rotz1 * rotx);
  //    // Flip vertex ordering
  //    if (!flip_faces) {
  //      for (auto& p : ps_start->indices) {
  //        std::reverse(p.begin(), p.end());
  //      }
  //    }
  //    builder.appendPolySet(*ps_start);

  //    auto ps_end = poly.tessellate();
  //    const Transform3d rotz2(angle_axis_degrees(node.start + node.angle, Vector3d::UnitZ()));
  //    ps_end->transform(rotz2 * rotx);
  //    if (flip_faces) {
  //      for (auto& p : ps_end->indices) {
  //        std::reverse(p.begin(), p.end());
  //      }
  //    }
  //    builder.appendPolySet(*ps_end);
  //  }

  // slice_stride is the number of vertices in a single ring
  size_t slice_stride = 0;
  for (const auto& o : poly.outlines()) {
    slice_stride += o.vertices.size();
  }
  int num_vertices = slice_stride * num_rings;
  std::vector<Vector3d> vertices;
  vertices.reserve(num_vertices);
  PolygonIndices indices;
  indices.reserve(slice_stride * num_rings * 2);  // sides + endcaps if needed

  // Calculate all vertices
  for (unsigned int j = 0; j < num_rings; ++j) {
    for (const auto& outline : poly.outlines()) {
      const double angle = node.start + j * node.angle / num_sections;  // start on the X axis
      fill_ring(std::back_inserter(vertices), outline, angle);
    }
  }

  // Calculate all indices
  for (unsigned int slice_idx = 1; slice_idx <= num_sections; slice_idx++) {
    int prev_slice = (slice_idx - 1) * slice_stride;
    int curr_slice = slice_idx * slice_stride;
    int curr_outline = 0;
    for (const auto& o : poly.outlines()) {
      for (int i = 1; i <= o.vertices.size(); ++i) {
        int curr_idx = curr_outline + (i % o.vertices.size());
        int prev_idx = curr_outline + i - 1;
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
      curr_outline += o.vertices.size();
    }
  }

  return assemblePolySetForManifold(poly, vertices, indices, closed, node.convexity,
                                    slice_stride * num_sections, flip_faces);

  //  for (const auto& o : poly.outlines()) {
  //    std::vector<Vector3d> rings[2];
  //    rings[0].reserve(o.vertices.size());
  //    rings[1].reserve(o.vertices.size());

  //    fill_ring(std::back_inserter(rings[0]), o, node.start, flip_faces); // first ring
  //    for (unsigned int j = 0; j < num_sections; ++j) {
  //      const double a = node.start + (j + 1) * node.angle / num_sections; // start on the X axis
  //      fill_ring(std::back_inserter(rings[(j + 1) % 2]), o, a, flip_faces);

  //      for (size_t i = 0; i < o.vertices.size(); ++i) {
  //        builder.appendPolygon({
  //                rings[j % 2][(i + 1) % o.vertices.size()],
  //                rings[(j + 1) % 2][(i + 1) % o.vertices.size()],
  //                rings[j % 2][i]
  //        });

  //        builder.appendPolygon({
  //                rings[(j + 1) % 2][(i + 1) % o.vertices.size()],
  //                rings[(j + 1) % 2][i],
  //                rings[j % 2][i]
  //        });
  //      }
  //    }
  //  }

  //  return builder.build();
}

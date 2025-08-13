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
#include "src/geometry/manifold/manifoldutils.h"

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
VectorOfVector2d alterprofile(VectorOfVector2d vertices, double scalex, double scaley, double origin_x,
                              double origin_y, double offset_x, double offset_y, double rot);

std::unique_ptr<Geometry> rotatePolygonSub(const RotateExtrudeNode& node, const Polygon2d& poly,
                                           int fragments, size_t fragstart, size_t fragend,
                                           bool flip_faces)
{
  double fact = (node.v[2] / node.angle) * (180.0 / G_PI);

  // # of sections. For closed rotations, # vertices is thus fragments*outline_size. For open
  // rotations # vertices is (fragments+1)*outline_size.
  const auto num_sections = fragend - fragstart;
  const bool closed = node.angle == 360 && node.v.norm() == 0;
  // # of rings of vertices
  const int num_rings = num_sections + (closed ? 0 : 1);

  // slice_stride is the number of vertices in a single ring
  size_t slice_stride = 0;
  int num_vertices = 0;
#ifdef ENABLE_PYTHON
  if (node.profile_func != NULL) {
    Outline2d outl = python_getprofile(node.profile_func, node.fn, 0);
    slice_stride += outl.vertices.size();
  } else
#endif
  {
    for (const auto& o : poly.outlines()) {
      slice_stride += o.vertices.size();
    }
  }
  num_vertices = slice_stride * num_rings;
  std::vector<Vector3d> vertices;
  vertices.reserve(num_vertices);
  PolygonIndices indices;
  indices.reserve(slice_stride * num_rings * 2);  // sides + endcaps if needed

  for (unsigned int j = fragstart; j <= fragend; ++j) {
    Vector3d dv = node.v * j / fragments;

    for (const auto& outline : poly.outlines()) {
      const double angle = node.start + j * node.angle / fragments;  // start on the X axis
      std::vector<Vector2d> vertices2d;
      double cur_twist = 0;
#ifdef ENABLE_PYTHON
      if (node.profile_func != NULL) {
        Outline2d lastFace;
        Outline2d curFace;
        Outline2d outl = python_getprofile(node.profile_func, node.fn, j / (double)fragments);
        vertices2d = outl.vertices;
      } else
#endif
        vertices2d = outline.vertices;
#ifdef ENABLE_PYTHON
      if (node.twist_func != NULL) cur_twist = python_doublefunc(node.twist_func, 0);
      else
#endif
        cur_twist = node.twist * j / fragments;
      vertices2d = alterprofile(vertices2d, 1.0, 1.0, node.origin_x, node.origin_y, node.offset_x,
                                node.offset_y, cur_twist);
      double xmid = NAN;
      if (node.method == "centered") {
        double xmin, xmax;
        xmin = xmax = vertices2d[0][0];
        for (const auto& v : vertices2d) {
          if (v[0] < xmin) xmin = v[0];
          if (v[0] > xmax) xmax = v[0];
        }
        xmid = (xmin + xmax) / 2;
      }

      for (const auto& v : vertices2d) {
        double tan_pitch = fact / (std::isnan(xmid) ? v[0] : xmid);
        //
        // cos(atan(x))=1/sqrt(1+x*x)
        // sin(atan(x))=x/sqrt(1+x*x)
        double cf = 1 / sqrt(1 + tan_pitch * tan_pitch);
        double sf = cf * tan_pitch;
        Vector3d centripedal = Vector3d(cos_degrees(angle), sin_degrees(angle), 0);
        Vector3d progress = Vector3d(-sin_degrees(angle) * cf, cos_degrees(angle) * cf, sf);
        Vector3d upwards = centripedal.cross(progress);
        Vector3d res = centripedal * v[0] + upwards * v[1] + dv;
        vertices.emplace_back(res);
      }  // vertices
    }  // outlines
  }  // fragments/rings

  // Calculate all indices
  for (unsigned int slice_idx = 1; slice_idx <= num_sections; slice_idx++) {
    const int prev_slice = (slice_idx - 1) * slice_stride;
    const int curr_slice = slice_idx * slice_stride;
    int curr_outline = 0;
    for (const auto& outline : poly.outlines()) {
      assert(outline.vertices.size() > 2);
      for (int i = 1; i <= outline.vertices.size(); ++i) {
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

std::unique_ptr<Geometry> rotatePolygon(const RotateExtrudeNode& node, const Polygon2d& poly)
{
  if (node.angle == 0) return nullptr;

  double min_x = 0;
  double max_x = 0;
  unsigned int fragments = 0;
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
  const auto num_sections = (unsigned int)std::ceil(
    fmax(Calc::get_fragments_from_r(max_x - min_x, 360.0, node.fn, node.fs, node.fa) *
           std::abs(node.angle) / 360,
         1));
  bool flip_faces = (min_x >= 0 && node.angle > 0) || (min_x < 0 && node.angle < 0);

  // check if its save to extrude
  bool safe = true;
  do {
    if (node.angle < 300) break;
    if (node.v.norm() == 0) break;
    if (node.v[2] / (node.angle / 360.0) > (max_x - min_x) * 1.5) break;
    safe = false;

  } while (false);
  if (safe) return rotatePolygonSub(node, poly, num_sections, 0, num_sections, flip_faces);

  // now create a fragment splitting plan
  size_t splits = ceil(node.angle / 300.0);
  fragments = num_sections;
  int fragstart = 0, fragend;
  std::unique_ptr<ManifoldGeometry> result = nullptr;

  for (size_t i = 0; i < splits; i++) {
    fragend = fragstart + (fragments / splits) + 1;
    if (fragend > fragments) fragend = fragments;
    std::unique_ptr<Geometry> part_u =
      rotatePolygonSub(node, poly, fragments, fragstart, fragend, flip_faces);
    std::shared_ptr<Geometry> part_s = std::move(part_u);
    std::shared_ptr<const ManifoldGeometry> term = ManifoldUtils::createManifoldFromGeometry(part_s);
    if (result == nullptr) result = std::make_unique<ManifoldGeometry>(*term);
    else *result = *result + *term;
    fragstart = fragend - 1;
  }
  return result;
}

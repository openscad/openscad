#include "rotate_extrude.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <memory>
#include <vector>

#include "core/RotateExtrudeNode.h"
#include "geometry/Geometry.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/Polygon2d.h"
#include "geometry/linalg.h"
#include "geometry/PolySet.h"
#include "utils/calc.h"
#include "utils/degree_trig.h"
#include "utils/printutils.h"

namespace {

void fill_ring(std::vector<Vector3d>& ring, const Outline2d& o, double a, bool flip)
{
  if (flip) {
    const unsigned int l = o.vertices.size() - 1;
    for (unsigned int i = 0; i < o.vertices.size(); ++i) {
      ring[i][0] = o.vertices[l - i][0] * cos_degrees(a);
      ring[i][1] = o.vertices[l - i][0] * sin_degrees(a);
      ring[i][2] = o.vertices[l - i][1];
    }
  } else {
    for (unsigned int i = 0; i < o.vertices.size(); ++i) {
      ring[i][0] = o.vertices[i][0] * cos_degrees(a);
      ring[i][1] = o.vertices[i][0] * sin_degrees(a);
      ring[i][2] = o.vertices[i][1];
    }
  }
}

}  // namespace

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
 
   PolySetBuilder builder;
   builder.setConvexity(node.convexity);
 
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
     LOG(message_group::Error, "all points for rotate_extrude() must have the same X coordinate sign (range is %1$.2f -> %2$.2f)", min_x, max_x);
     return nullptr;
   }
 
   fragments = (unsigned int)std::ceil(fmax(Calc::get_fragments_from_r(max_x - min_x, node.fn, node.fs, node.fa) * std::abs(node.angle) / 360, 1));
 
   const bool flip_faces = (min_x >= 0 && node.angle > 0) || (min_x < 0 && node.angle < 0);
 
   // If not going all the way around, we have to create faces on each end.
   if (node.angle != 360) {
     const auto ps_start = poly.tessellate(); // starting face
     const Transform3d rotx(angle_axis_degrees(90, Vector3d::UnitX()));
     const Transform3d rotz1(angle_axis_degrees(node.start, Vector3d::UnitZ()));
     ps_start->transform(rotz1 * rotx);
     // Flip vertex ordering
     if (!flip_faces) {
       for (auto& p : ps_start->indices) {
         std::reverse(p.begin(), p.end());
       }
     }
     builder.appendPolySet(*ps_start);
 
     auto ps_end = poly.tessellate();
     const Transform3d rotz2(angle_axis_degrees(node.start + node.angle, Vector3d::UnitZ()));
     ps_end->transform(rotz2 * rotx);
     if (flip_faces) {
       for (auto& p : ps_end->indices) {
         std::reverse(p.begin(), p.end());
       }
     }
     builder.appendPolySet(*ps_end);
   }
 
   for (const auto& o : poly.outlines()) {
     std::vector<Vector3d> rings[2];
     rings[0].resize(o.vertices.size());
     rings[1].resize(o.vertices.size());
 
     fill_ring(rings[0], o, node.start, flip_faces); // first ring
     for (unsigned int j = 0; j < fragments; ++j) {
       const double a = node.start + (j + 1) * node.angle / fragments; // start on the X axis
       fill_ring(rings[(j + 1) % 2], o, a, flip_faces);
 
       for (size_t i = 0; i < o.vertices.size(); ++i) {
         builder.appendPolygon({
                 rings[j % 2][(i + 1) % o.vertices.size()],
                 rings[(j + 1) % 2][(i + 1) % o.vertices.size()],
                 rings[j % 2][i]
         });
 
         builder.appendPolygon({
                 rings[(j + 1) % 2][(i + 1) % o.vertices.size()],
                 rings[(j + 1) % 2][i],
                 rings[j % 2][i]
         });
       }
     }
   }
 
   return builder.build();
 }

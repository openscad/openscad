// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"

#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/tags.h>

namespace PMP = CGAL::Polygon_mesh_processing;

namespace CGALUtils {

/**
 * Repairs a mesh by orienting its faces, and if needed removes self-intersecting faces and closes holes.
 */
template <class TriangleMesh>
void repairMesh(TriangleMesh& tm)
{
  typedef boost::graph_traits<TriangleMesh> GT;
  typedef typename GT::face_descriptor face_descriptor;

  try {
    // Note: PMP::orient can corrupt models and cause cataclysmic memory leaks
    // (try testdata/scad/3D/issues/issue1105d.scad for instance), so we try
    // the stricter PMP::orient_to_bound_a_volume first.
    PMP::orient_to_bound_a_volume(tm);
  } catch (const CGAL::Failure_exception& e) {
    LOG(message_group::Warning, "[repair] Failed to orient mesh: it's either unclosed or self-intersecting. Attempting repair of either cases.");

    size_t removedFaceCount = 0;
    std::vector<std::pair<face_descriptor, face_descriptor>> selfIntersectionPairs;
    PMP::self_intersections<CGAL::Parallel_if_available_tag>(faces(tm), tm, back_inserter(selfIntersectionPairs));
    if (!selfIntersectionPairs.empty()) {
      for (auto& p : selfIntersectionPairs) {
        auto& f = p.first;
        if (!tm.is_removed(f)) {
          tm.remove_face(f);
          removedFaceCount++;
        }
      }
      if (removedFaceCount) {
        LOG(message_group::Warning, "[repair] Removed %1$lu self-intersecting faces", removedFaceCount);
      }
    }

    size_t addedFaceCount = 0;
    if (!CGAL::is_closed(tm)) {
      size_t holeCount = 0;
      size_t facesBefore = num_faces(tm);
      for (auto he : tm.halfedges()) {
        if (!tm.face(he).is_valid()) {
          PMP::triangulate_hole(tm, he);
          holeCount++;
        }
      }
      LOG(message_group::Warning, "[repair] Closed %1$lu holes with %2$lu new faces", holeCount, num_faces(tm) - facesBefore);
    }
    if (removedFaceCount || addedFaceCount) {
      TriangleMesh copy;
      copyMesh(tm, copy);
      tm = copy;
    }
    // Less strict than PMP::orient_to_bound_a_volume:
    PMP::orient(tm);
  }
}

template void repairMesh(CGAL_HybridMesh& tm);
template void repairMesh(CGAL_EpickMesh& tm);

template <typename Polyhedron>
void reverseFaceOrientations(Polyhedron& polyhedron)
{
  PMP::reverse_face_orientations(polyhedron);
}

template void reverseFaceOrientations(CGAL_HybridMesh& polyhedron);

} // namespace CGALUtils


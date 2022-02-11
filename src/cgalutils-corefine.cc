// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

typedef CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> CGAL_HybridSurfaceMesh;

#if FAST_CSG_KERNEL_IS_LAZY

/*! Visitor that forces exact numbers for the vertices of all the faces created during corefinement.
 */
template <typename TriangleMesh>
struct ExactLazyNumbersVisitor
  : public CGAL::Polygon_mesh_processing::Corefinement::Default_visitor<TriangleMesh> {
  typedef typename TriangleMesh::Face_index face_descriptor;

  face_descriptor split_face;
  std::vector<face_descriptor> created_faces;

  void before_subface_creations(face_descriptor f_split, TriangleMesh &tm)
  {
    split_face = f_split;
    created_faces.clear();
  }

  void after_subface_creations(TriangleMesh &mesh)
  {
    for (auto &fi : created_faces) {
      CGAL::Vertex_around_face_iterator<TriangleMesh> vbegin, vend;
      for (boost::tie(vbegin, vend) = vertices_around_face(mesh.halfedge(fi), mesh); vbegin != vend;
           ++vbegin) {
        auto &v = mesh.point(*vbegin);
        CGAL::exact(v.x());
        CGAL::exact(v.y());
        CGAL::exact(v.z());
      }
    }
    created_faces.clear();
  }
  void after_subface_created(face_descriptor fi, TriangleMesh &tm) { created_faces.push_back(fi); }
};

#endif // FAST_CSG_KERNEL_IS_LAZY

#define COREFINEMENT_IMPL_SIMPLE(fn) \
  return fn(lhs, rhs, out, \
    CGAL::parameters::throw_on_self_intersection(throwOnSelfIntersection));
#define COREFINEMENT_IMPL_CALLBACKS(fn) \
  return fn(lhs, rhs, out, \
    CGAL::Polygon_mesh_processing::parameters::visitor( \
        ExactLazyNumbersVisitor<CGAL::Surface_mesh<CGAL::Point_3<K>>>()), \
    CGAL::Polygon_mesh_processing::parameters::visitor( \
        ExactLazyNumbersVisitor<CGAL::Surface_mesh<CGAL::Point_3<K>>>()), \
    CGAL::parameters::throw_on_self_intersection(throwOnSelfIntersection));

#if FAST_CSG_KERNEL_IS_LAZY
#define COREFINEMENT_IMPL(fn) \
  if (Feature::ExperimentalFastCsgExactCorefinementCallback.is_enabled()) { \
    COREFINEMENT_IMPL_CALLBACKS(fn); \
  } else { \
    COREFINEMENT_IMPL_SIMPLE(fn); \
  }
#else
#define COREFINEMENT_IMPL(fn) COREFINEMENT_IMPL_SIMPLE(fn);
#endif // FAST_CSG_KERNEL_IS_LAZY

template <typename K>
bool corefineAndComputeUnion(
  CGAL::Surface_mesh<CGAL::Point_3<K>>& lhs,
  CGAL::Surface_mesh<CGAL::Point_3<K>>& rhs,
  CGAL::Surface_mesh<CGAL::Point_3<K>>& out,
  bool throwOnSelfIntersection)
{
  COREFINEMENT_IMPL(CGAL::Polygon_mesh_processing::corefine_and_compute_union);
}

template <typename K>
bool corefineAndComputeIntersection(
  CGAL::Surface_mesh<CGAL::Point_3<K>>& lhs,
  CGAL::Surface_mesh<CGAL::Point_3<K>>& rhs,
  CGAL::Surface_mesh<CGAL::Point_3<K>>& out,
  bool throwOnSelfIntersection)
{
  COREFINEMENT_IMPL(CGAL::Polygon_mesh_processing::corefine_and_compute_intersection);
}

template <typename K>
bool corefineAndComputeDifference(
  CGAL::Surface_mesh<CGAL::Point_3<K>>& lhs,
  CGAL::Surface_mesh<CGAL::Point_3<K>>& rhs,
  CGAL::Surface_mesh<CGAL::Point_3<K>>& out,
  bool throwOnSelfIntersection)
{
  COREFINEMENT_IMPL(CGAL::Polygon_mesh_processing::corefine_and_compute_difference);
}

template bool corefineAndComputeUnion(
  CGAL_HybridSurfaceMesh& lhs, CGAL_HybridSurfaceMesh& rhs, CGAL_HybridSurfaceMesh& out, bool throwOnSelfIntersection);
template bool corefineAndComputeIntersection(
  CGAL_HybridSurfaceMesh& lhs, CGAL_HybridSurfaceMesh& rhs, CGAL_HybridSurfaceMesh& out, bool throwOnSelfIntersection);
template bool corefineAndComputeDifference(
  CGAL_HybridSurfaceMesh& lhs, CGAL_HybridSurfaceMesh& rhs, CGAL_HybridSurfaceMesh& out, bool throwOnSelfIntersection);

} // namespace CGALUtils


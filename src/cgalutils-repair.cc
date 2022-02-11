// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#ifdef ENABLE_CGAL_REMESHING
#include <CGAL/Polygon_mesh_processing/remesh_planar_patches.h>
#endif
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Surface_mesh.h>


namespace CGALUtils {

template <typename M>
bool removeSelfIntersections(M &mesh)
{
  return CGAL::Polygon_mesh_processing::experimental::remove_self_intersections(mesh);
}

template bool removeSelfIntersections(CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>> &mesh);

} // namespace CGALUtils


// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#ifdef ENABLE_CGAL_REMESHING
#include <CGAL/Polygon_mesh_processing/remesh_planar_patches.h>
#endif
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

#ifdef ENABLE_CGAL_REMESHING
template <typename M>
bool remeshPlanarPatches(M &mesh)
{
  CGAL::Polygon_mesh_processing::remesh_planar_patches(mesh);
}

template bool remeshPlanarPatches(CGAL::Surface_mesh<CGAL::Point_3<CGAL::Epeck>> &mesh);
#endif

} // namespace CGALUtils


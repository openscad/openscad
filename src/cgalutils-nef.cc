
// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"
#include <CGAL/boost/graph/convert_nef_polyhedron_to_polygon_mesh.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

#ifdef FAST_CSG_AVAILABLE

template <typename K>
void inPlaceNefUnion(CGAL::Nef_polyhedron_3<K> &lhs, const CGAL::Nef_polyhedron_3<K> &rhs)
{
	lhs += rhs;
}

template <typename K>
void inPlaceNefDifference(CGAL::Nef_polyhedron_3<K> &lhs, const CGAL::Nef_polyhedron_3<K> &rhs)
{
	lhs -= rhs;
}

template <typename K>
void inPlaceNefIntersection(CGAL::Nef_polyhedron_3<K> &lhs, const CGAL::Nef_polyhedron_3<K> &rhs)
{
	lhs *= rhs;
}

template void inPlaceNefUnion(CGAL::Nef_polyhedron_3<CGAL_HybridKernel3> &lhs,
															const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3> &rhs);
template void inPlaceNefDifference(CGAL::Nef_polyhedron_3<CGAL_HybridKernel3> &lhs,
																	 const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3> &rhs);
template void inPlaceNefIntersection(CGAL::Nef_polyhedron_3<CGAL_HybridKernel3> &lhs,
																		 const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3> &rhs);

template <typename K>
shared_ptr<CGAL::Nef_polyhedron_3<K>> createNefPolyhedronFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<K>> &mesh)
{
  return make_shared<CGAL::Nef_polyhedron_3<K>>(mesh);
}

template shared_ptr<CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>> createNefPolyhedronFromMesh(const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &mesh);

template <typename K>
void convertNefPolyhedronToMesh(const CGAL::Nef_polyhedron_3<K>& nef, CGAL::Surface_mesh<CGAL::Point_3<K>> &mesh)
{
  CGAL::convert_nef_polyhedron_to_polygon_mesh(nef, mesh, /* triangulate_all_faces */ true);
}

template void convertNefPolyhedronToMesh(const CGAL::Nef_polyhedron_3<CGAL_Kernel3>& nef, CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> &mesh);
#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
template void convertNefPolyhedronToMesh(const CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>& nef, CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &mesh);
#endif

#endif // FAST_CSG_AVAILABLE

} // namespace CGALUtils

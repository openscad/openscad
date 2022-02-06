// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

typedef CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> CGAL_HybridSurfaceMesh;

template <typename K>
bool corefineAndComputeUnion(
	CGAL::Surface_mesh<CGAL::Point_3<K>> &lhs,
	CGAL::Surface_mesh<CGAL::Point_3<K>> &rhs,
	CGAL::Surface_mesh<CGAL::Point_3<K>> &out)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_union(lhs, rhs, out);
}

template <typename K>
bool corefineAndComputeIntersection(
	CGAL::Surface_mesh<CGAL::Point_3<K>> &lhs,
	CGAL::Surface_mesh<CGAL::Point_3<K>> &rhs,
	CGAL::Surface_mesh<CGAL::Point_3<K>> &out)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(lhs, rhs, out);
}

template <typename K>
bool corefineAndComputeDifference(
	CGAL::Surface_mesh<CGAL::Point_3<K>> &lhs,
	CGAL::Surface_mesh<CGAL::Point_3<K>> &rhs,
	CGAL::Surface_mesh<CGAL::Point_3<K>> &out)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_difference(lhs, rhs, out);
}

template bool corefineAndComputeUnion(
	CGAL_HybridSurfaceMesh &lhs, CGAL_HybridSurfaceMesh &rhs, CGAL_HybridSurfaceMesh &out);
template bool corefineAndComputeIntersection(
	CGAL_HybridSurfaceMesh &lhs, CGAL_HybridSurfaceMesh &rhs, CGAL_HybridSurfaceMesh &out);
template bool corefineAndComputeDifference(
	CGAL_HybridSurfaceMesh &lhs, CGAL_HybridSurfaceMesh &rhs, CGAL_HybridSurfaceMesh &out);

} // namespace CGALUtils


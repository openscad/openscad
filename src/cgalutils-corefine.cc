// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/Polygon_mesh_processing/corefinement.h>

namespace CGALUtils {

template <typename K>
bool corefineAndComputeUnion(CGAL::Polyhedron_3<K> &lhs, CGAL::Polyhedron_3<K> &rhs,
														 CGAL::Polyhedron_3<K> &out)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_union(lhs, rhs, out);
}

template <typename K>
bool corefineAndComputeIntersection(CGAL::Polyhedron_3<K> &lhs, CGAL::Polyhedron_3<K> &rhs,
																		CGAL::Polyhedron_3<K> &out)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(lhs, rhs, out);
}

template <typename K>
bool corefineAndComputeDifference(CGAL::Polyhedron_3<K> &lhs, CGAL::Polyhedron_3<K> &rhs,
																	CGAL::Polyhedron_3<K> &out)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_difference(lhs, rhs, out);
}

template bool corefineAndComputeUnion(CGAL::Polyhedron_3<CGAL_HybridKernel3> &lhs,
																			CGAL::Polyhedron_3<CGAL_HybridKernel3> &rhs,
																			CGAL::Polyhedron_3<CGAL_HybridKernel3> &out);
template bool corefineAndComputeIntersection(CGAL::Polyhedron_3<CGAL_HybridKernel3> &lhs,
																						 CGAL::Polyhedron_3<CGAL_HybridKernel3> &rhs,
																						 CGAL::Polyhedron_3<CGAL_HybridKernel3> &out);
template bool corefineAndComputeDifference(CGAL::Polyhedron_3<CGAL_HybridKernel3> &lhs,
																					 CGAL::Polyhedron_3<CGAL_HybridKernel3> &rhs,
																					 CGAL::Polyhedron_3<CGAL_HybridKernel3> &out);

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

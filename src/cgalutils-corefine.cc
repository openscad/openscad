// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/Polygon_mesh_processing/corefinement.h>

namespace CGALUtils {

template <typename K>
bool corefineAndComputeUnion(CGAL::Polyhedron_3<K> &destination, CGAL::Polyhedron_3<K> &other)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_union(destination, other, destination);
}

template <typename K>
bool corefineAndComputeIntersection(CGAL::Polyhedron_3<K> &destination,
																		CGAL::Polyhedron_3<K> &other)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(destination, other, destination);
}

template <typename K>
bool corefineAndComputeDifference(CGAL::Polyhedron_3<K> &destination, CGAL::Polyhedron_3<K> &other)
{
	return CGAL::Polygon_mesh_processing::corefine_and_compute_difference(destination, other, destination);
}

template bool corefineAndComputeUnion(CGAL::Polyhedron_3<CGAL_HybridKernel3> &destination,
																			CGAL::Polyhedron_3<CGAL_HybridKernel3> &other);
template bool corefineAndComputeIntersection(CGAL::Polyhedron_3<CGAL_HybridKernel3> &destination,
																						 CGAL::Polyhedron_3<CGAL_HybridKernel3> &other);
template bool corefineAndComputeDifference(CGAL::Polyhedron_3<CGAL_HybridKernel3> &destination,
																					 CGAL::Polyhedron_3<CGAL_HybridKernel3> &other);

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

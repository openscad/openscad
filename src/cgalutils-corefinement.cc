// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/Polygon_mesh_processing/corefinement.h>

namespace CGALUtils {

void corefineAndComputeUnion(CGALHybridPolyhedron::polyhedron_t &destination,
														 CGALHybridPolyhedron::polyhedron_t &other)
{
	CGAL::Polygon_mesh_processing::corefine_and_compute_union(destination, other, destination);
}
void corefineAndComputeIntersection(CGALHybridPolyhedron::polyhedron_t &destination,
																		CGALHybridPolyhedron::polyhedron_t &other)
{
	CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(destination, other, destination);
}
void corefineAndComputeDifference(CGALHybridPolyhedron::polyhedron_t &destination,
																	CGALHybridPolyhedron::polyhedron_t &other)
{
	CGAL::Polygon_mesh_processing::corefine_and_compute_difference(destination, other, destination);
}

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

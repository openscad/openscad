
// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

namespace CGALUtils {

void inPlaceNefUnion(CGAL::Nef_polyhedron_3<CGAL::Epeck> &lhs,
										 const CGAL::Nef_polyhedron_3<CGAL::Epeck> &rhs)
{
	lhs += rhs;
}

void inPlaceNefDifference(CGAL::Nef_polyhedron_3<CGAL::Epeck> &lhs,
													const CGAL::Nef_polyhedron_3<CGAL::Epeck> &rhs)
{
	lhs -= rhs;
}

void inPlaceNefIntersection(CGAL::Nef_polyhedron_3<CGAL::Epeck> &lhs,
														const CGAL::Nef_polyhedron_3<CGAL::Epeck> &rhs)
{
	lhs *= rhs;
}

void convertNefToPolyhedron(const CGAL_Nef_polyhedron3 &nef, CGAL_Polyhedron &polyhedron)
{
	nef.convert_to_polyhedron(polyhedron);
}

void convertNefToPolyhedron(const CGAL::Nef_polyhedron_3<CGAL::Epeck> &nef,
														CGAL::Polyhedron_3<CGAL::Epeck> &polyhedron)
{
	nef.convert_to_polyhedron(polyhedron);
}

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

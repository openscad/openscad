
// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

namespace CGALUtils {

#ifdef FAST_CSG_AVAILABLE

void inPlaceNefMinkowski(CGAL::Nef_polyhedron_3<CGAL::Epeck> &lhs,
												 CGAL::Nef_polyhedron_3<CGAL::Epeck> &rhs)
{
	lhs = CGAL::minkowski_sum_3(lhs, rhs);
}

#endif // FAST_CSG_AVAILABLE

} // namespace CGALUtils

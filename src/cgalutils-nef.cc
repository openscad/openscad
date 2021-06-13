
// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

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


#endif // FAST_CSG_AVAILABLE

} // namespace CGALUtils

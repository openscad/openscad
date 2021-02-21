// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Surface_mesh.h>
#include "scoped_timer.h"

namespace CGALUtils {

template <typename Polyhedron>
bool isClosed(Polyhedron &polyhedron)
{
  SCOPED_PERFORMANCE_TIMER("isClosed");

	return CGAL::is_closed(polyhedron);
}

template bool isClosed(CGAL::Polyhedron_3<CGAL_HybridKernel3> &polyhedron);
template bool isClosed(CGAL::Surface_mesh<CGAL::Point_3<CGAL_Kernel3>> &polyhedron);
#ifdef FAST_CSG_AVAILABLE_WITH_DIFFERENT_KERNEL
template bool isClosed(CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &polyhedron);
#endif

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

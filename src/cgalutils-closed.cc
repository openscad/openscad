// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgal.h"
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"


#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Surface_mesh.h>
#include "scoped_timer.h"

namespace CGALUtils {

template <typename Polyhedron>
bool isClosed(const Polyhedron &p)
{
  SCOPED_PERFORMANCE_TIMER("isClosed");

	return CGAL::is_closed(p);
}

template bool isClosed(const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &p);

} // namespace CGALUtils


// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/boost/graph/helpers.h>

namespace CGALUtils {

template <typename Polyhedron>
bool isClosed(Polyhedron &polyhedron)
{
	return CGAL::is_closed(polyhedron);
}

template bool isClosed(CGAL::Polyhedron_3<CGAL_HybridKernel3> &polyhedron);

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

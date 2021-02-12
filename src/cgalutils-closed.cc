// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/boost/graph/helpers.h>

namespace CGALUtils {

bool isClosed(CGAL::Polyhedron_3<CGAL::Epeck> &polyhedron)
{
	return CGAL::is_closed(polyhedron);
}

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

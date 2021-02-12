// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>

namespace PMP = CGAL::Polygon_mesh_processing;

namespace CGALUtils {

void triangulateFaces(CGAL::Polyhedron_3<CGAL::Epeck> &polyhedron)
{
	PMP::triangulate_faces(polyhedron);
}

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

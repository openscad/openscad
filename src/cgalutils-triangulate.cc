// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Surface_mesh.h>

namespace PMP = CGAL::Polygon_mesh_processing;

namespace CGALUtils {

template <typename Polyhedron>
void triangulateFaces(Polyhedron &polyhedron)
{
	PMP::triangulate_faces(polyhedron);
}

template void triangulateFaces(CGAL::Polyhedron_3<CGAL_HybridKernel3> &polyhedron);
template void triangulateFaces(CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &polyhedron);

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

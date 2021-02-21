// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include "CGALHybridPolyhedron.h"

#ifdef FAST_CSG_AVAILABLE

#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

template <typename Polyhedron>
void orientToBoundAVolume(Polyhedron &polyhedron)
{
	CGAL::Polygon_mesh_processing::orient_to_bound_a_volume(polyhedron);
}

template void orientToBoundAVolume(CGAL::Polyhedron_3<CGAL_HybridKernel3> &polyhedron);
template void orientToBoundAVolume(CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>> &polyhedron);

} // namespace CGALUtils

#endif // FAST_CSG_AVAILABLE

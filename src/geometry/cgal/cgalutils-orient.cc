// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/cgal/cgalutils.h"

#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

template <typename SurfaceMesh>
void orientToBoundAVolume(SurfaceMesh& mesh)
{
  CGAL::Polygon_mesh_processing::orient_to_bound_a_volume(mesh);
}

template void orientToBoundAVolume(CGAL_DoubleMesh& polyhedron);

}  // namespace CGALUtils

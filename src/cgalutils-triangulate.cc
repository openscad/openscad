// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"

#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

template <typename Polyhedron>
void triangulateFaces(Polyhedron& polyhedron)
{
#ifndef FAST_CSG_DISABLED_TRIANGULATION_BUG
  CGAL::Polygon_mesh_processing::triangulate_faces(polyhedron);
#endif
}

template void triangulateFaces(CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& polyhedron);

} // namespace CGALUtils


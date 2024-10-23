// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/cgal/cgalutils.h"

#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

template <typename Polyhedron>
bool isClosed(const Polyhedron& p)
{
  return CGAL::is_closed(p);
}

template bool isClosed(const CGAL_HybridMesh& p);
template bool isClosed(const CGAL_DoubleMesh& p);

} // namespace CGALUtils


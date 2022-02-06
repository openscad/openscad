// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"

#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Surface_mesh.h>

namespace CGALUtils {

template <typename Polyhedron>
bool isClosed(const Polyhedron& p)
{
  return CGAL::is_closed(p);
}

template bool isClosed(const CGAL::Surface_mesh<CGAL::Point_3<CGAL_HybridKernel3>>& p);

} // namespace CGALUtils


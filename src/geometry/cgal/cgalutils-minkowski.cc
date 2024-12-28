
// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/cgal/cgalutils.h"

namespace CGALUtils {

template <typename K>
void inPlaceNefMinkowski(
  CGAL::Nef_polyhedron_3<K>& lhs,
  CGAL::Nef_polyhedron_3<K>& rhs)
{
  lhs = CGAL::minkowski_sum_3(lhs, rhs);
}

} // namespace CGALUtils

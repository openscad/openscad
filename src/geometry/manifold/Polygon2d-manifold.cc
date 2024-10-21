#include "geometry/Polygon2d.h"
#include "geometry/PolySet.h"
#include "utils/printutils.h"


#ifndef ENABLE_CGAL
/*!
   Triangulates this polygon2d and returns a 2D-in-3D PolySet.
 */
std::unique_ptr<PolySet> Polygon2d::tessellate() const
{
  PRINTDB("Polygon2d::tessellate(): %d outlines", this->outlines().size());
  return std::make_unique<PolySet>(*this);
}
#endif

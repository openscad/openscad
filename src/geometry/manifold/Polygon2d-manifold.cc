#include "Polygon2d.h"
#include "PolySet.h"
#include "printutils.h"

#include <iostream>

#ifndef ENABLE_CGAL
/*!
   Triangulates this polygon2d and returns a 2D-in-3D PolySet.
 */
PolySet *Polygon2d::tessellate() const
{
  PRINTDB("Polygon2d::tessellate(): %d outlines", this->outlines().size());
  auto polyset = new PolySet(*this);
  return polyset;
}
#endif

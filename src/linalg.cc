#include "linalg.h"

BoundingBox operator*(const Transform3d &m, const BoundingBox &box)
{
  BoundingBox bbox(box);
  bbox.extend(m * box.min());
  bbox.extend(m * box.max());
  return bbox;
}



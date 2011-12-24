#include "linalg.h"

BoundingBox operator*(const Transform3d &m, const BoundingBox &box)
{
  // FIXME - stopgap while waiting for real version
  BoundingBox bbox(box);
  bbox.extend(m * box.min());
  bbox.extend(m * box.max());
  return bbox;
}



#include "linalg.h"
#include <boost/math/special_functions/fpclassify.hpp>

// FIXME: We can achieve better pruning by either:
// o Recalculate the box based on the transformed object
// o Store boxes are oriented bounding boxes and implement oriented
//   bbox intersection

// FIXME: This function can be generalized, but we don't need it atm.

/*!
	Transforms the given bounding box by transforming each of its 8 vertices.
	Returns a new bounding box.
*/
BoundingBox operator*(const Transform3d &m, const BoundingBox &box)
{
	if (box.isEmpty()) return box;
	BoundingBox newbox;
	Vector3d boxvec[2] = { box.min(), box.max() };
	for (int k=0;k<2;k++) {
		for (int j=0;j<2;j++) {
			for (int i=0;i<2;i++) {
				newbox.extend(m * Vector3d(boxvec[i][0], boxvec[j][1], boxvec[k][2]));
			}
		}
	}
	return newbox;
}

bool matrix_contains_infinity( const Transform3d &m )
{
  for (int i=0;i<m.matrix().rows();i++) {
    for (int j=0;j<m.matrix().cols();j++) {
      if ((boost::math::isinf)(m(i,j))) return true;
    }
  }
	return false;
}

bool matrix_contains_nan( const Transform3d &m )
{
  for (int i=0;i<m.matrix().rows();i++) {
    for (int j=0;j<m.matrix().cols();j++) {
      if ((boost::math::isnan)(m(i,j))) return true;
    }
  }
	return false;
}

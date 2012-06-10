/* 
   Work around bugs in MSVC compiler with Eigen AlignmentBox
   bbox.min and bbox.max will fail with Syntax Errors if placed inside
   of cgalpngtest.cc
*/

#include "linalg.h"
#include "polyutils.h"

Vector3d getBoundingCenter(BoundingBox bbox)
{
	Vector3d center = (bbox.min() + bbox.max()) / 2;
	return center; // Vector3d(0,0,0);
}

double getBoundingRadius(BoundingBox bbox)
{
	double radius = (bbox.max() - bbox.min()).norm() / 2;
	return radius; // 0;
}



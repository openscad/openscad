#ifndef POLYGON2D_CGAL_H_
#define POLYGON2D_CGAL_H_

#include "Polygon2d.h"
#include "cgal.h"
#include "CGAL_Nef_polyhedron.h"

namespace Polygon2DCGAL {
	CGAL_Nef_polyhedron toNefPolyhedron();
};

#endif

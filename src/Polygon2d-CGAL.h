#pragma once

#include "Polygon2d.h"
#include "cgal.h"
#include "CGAL_Nef_polyhedron.h"

namespace Polygon2DCGAL {
	CSGIF_polyhedron toNefPolyhedron();
};

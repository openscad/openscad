#ifndef CGALUTILS_H_
#define CGALUTILS_H_

#include <cgalfwd.h>

class PolySet *createPolySetFromPolyhedron(const CGAL_Polyhedron &p);
CGAL_Polyhedron *createPolyhedronFromPolySet(const class PolySet &ps);
CGAL_Iso_cuboid_3 bounding_box( const CGAL_Nef_polyhedron3 &N );
CGAL_Iso_rectangle_2e bounding_box( const CGAL_Nef_polyhedron2 &N );

#endif

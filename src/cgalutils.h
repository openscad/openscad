#ifndef CGALUTILS_H_
#define CGALUTILS_H_

#include <cgalfwd.h>

class PolySet *createPolySetFromPolyhedron(const CGAL_Polyhedron &p);
CGAL_Polyhedron *createPolyhedronFromPolySet(const class PolySet &ps);

#endif

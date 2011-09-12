#ifndef CGAL_NEF_POLYHEDRON_H_
#define CGAL_NEF_POLYHEDRON_H_

#ifdef ENABLE_CGAL

#include "cgalfwd.h"
#include "memory.h"

class CGAL_Nef_polyhedron
{
public:
	CGAL_Nef_polyhedron() : dim(0) {}
	CGAL_Nef_polyhedron(CGAL_Nef_polyhedron2 *p) : dim(2), p2(p) {}
	CGAL_Nef_polyhedron(CGAL_Nef_polyhedron3 *p) : dim(3), p3(p) {}
	~CGAL_Nef_polyhedron() {}

	bool empty() const { return (dim == 0 || (!p2 && !p3)); }
	CGAL_Nef_polyhedron &operator+=(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron &operator*=(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron &operator-=(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron &minkowski(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron copy() const;
	int weight() const;
	class PolySet *convertToPolyset();
	class DxfData *convertToDxfData() const;

	int dim;
	shared_ptr<CGAL_Nef_polyhedron2> p2;
	shared_ptr<CGAL_Nef_polyhedron3> p3;
};

#endif /* ENABLE_CGAL */

#endif

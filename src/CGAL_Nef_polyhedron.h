#ifndef CGAL_NEF_POLYHEDRON_H_
#define CGAL_NEF_POLYHEDRON_H_

#include "cgal.h"
#include "memory.h"
#include <string>
#include "linalg.h"

class CGAL_Nef_polyhedron
{
public:
	CGAL_Nef_polyhedron(int dim = 0) : dim(dim) {}
	CGAL_Nef_polyhedron(CGAL_Nef_polyhedron2 *p);
	CGAL_Nef_polyhedron(CGAL_Nef_polyhedron3 *p);
	~CGAL_Nef_polyhedron() {}

  // Empty means it is a geometric node which has zero area/volume
	bool isEmpty() const { return (dim > 0 && !p2 && !p3); }
  // Null means the node doesn't contain any geometry (for whatever reason)
	bool isNull() const { return !p2 && !p3; }
	void reset() { dim=0; p2.reset(); p3.reset(); }
	CGAL_Nef_polyhedron &operator+=(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron &operator*=(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron &operator-=(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron &minkowski(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron copy() const;
	std::string dump() const;
	int weight() const;
	class PolySet *convertToPolyset();
	class DxfData *convertToDxfData() const;
	void transform( const Transform3d &matrix );
	int dim;
	shared_ptr<CGAL_Nef_polyhedron2> p2;
	shared_ptr<CGAL_Nef_polyhedron3> p3;
};

#endif

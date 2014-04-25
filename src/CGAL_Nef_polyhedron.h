#ifndef CGAL_NEF_POLYHEDRON_H_
#define CGAL_NEF_POLYHEDRON_H_

#include "Geometry.h"
#include "cgal.h"
#include "memory.h"
#include <string>
#include "linalg.h"

class CGAL_Nef_polyhedron : public Geometry
{
public:
	CGAL_Nef_polyhedron(CGAL_Nef_polyhedron3 *p = NULL);
	~CGAL_Nef_polyhedron() {}

	virtual size_t memsize() const;
	// FIXME: Implement, but we probably want a high-resolution BBox..
	virtual BoundingBox getBoundingBox() const { assert(false && "not implemented"); }
	virtual std::string dump() const;
	virtual unsigned int getDimension() const { return 3; }
  // Empty means it is a geometric node which has zero area/volume
	virtual bool isEmpty() const { return !p3; }

	void reset() { p3.reset(); }
	CGAL_Nef_polyhedron &operator+=(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron &operator*=(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron &operator-=(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron &minkowski(const CGAL_Nef_polyhedron &other);
	CGAL_Nef_polyhedron *copy() const;
	class PolySet *convertToPolyset() const;
	void transform( const Transform3d &matrix );
	shared_ptr<CGAL_Nef_polyhedron3> p3;
};

#endif

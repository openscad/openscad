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
	CGAL_Nef_polyhedron(int dim = 0) : dim(dim) {}
	CGAL_Nef_polyhedron(CGAL_Nef_polyhedron2 *p);
	CGAL_Nef_polyhedron(CGAL_Nef_polyhedron3 *p);
	~CGAL_Nef_polyhedron() {}

	virtual size_t memsize() const;
	virtual BoundingBox getBoundingBox() const {}; // FIXME: Implement
	virtual std::string dump() const;
	virtual unsigned int getDimension() const { return this->dim; }
	
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
	class PolySet *convertToPolyset() const;
	class DxfData *convertToDxfData() const;
	class Polygon2d *convertToPolygon2d() const;
	void transform( const Transform3d &matrix );
	shared_ptr<CGAL_Nef_polyhedron2> p2;
	shared_ptr<CGAL_Nef_polyhedron3> p3;
protected:
	int dim;
};

#endif

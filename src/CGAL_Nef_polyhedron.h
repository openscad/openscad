#pragma once

#include "Geometry.h"
#include "cgal.h"
#include "memory.h"
#include <string>
#include "linalg.h"

class CSGIF_polyhedron : public Geometry
{
public:
	CSGIF_polyhedron(CGAL_Nef_polyhedron3 *p = NULL);
	CSGIF_polyhedron(const CSGIF_polyhedron &src);
	~CSGIF_polyhedron() {}

	virtual size_t memsize() const;
	// FIXME: Implement, but we probably want a high-resolution BBox..
	virtual BoundingBox getBoundingBox() const { assert(false && "not implemented"); }
	virtual std::string dump() const;
	virtual unsigned int getDimension() const { return 3; }
  // Empty means it is a geometric node which has zero area/volume
	virtual bool isEmpty() const;
	virtual Geometry *copy() const { return new CSGIF_polyhedron(*this); }

	void reset() { p3.reset(); }
	CSGIF_polyhedron &operator+=(const CSGIF_polyhedron &other);
	CSGIF_polyhedron &operator*=(const CSGIF_polyhedron &other);
	CSGIF_polyhedron &operator-=(const CSGIF_polyhedron &other);
	CSGIF_polyhedron &minkowski(const CSGIF_polyhedron &other);
// FIXME: Deprecated by CGALUtils::createPolySetFromNefPolyhedron3
//	class PolySet *convertToPolyset() const;
	void transform( const Transform3d &matrix );
	void resize(Vector3d newsize, const Eigen::Matrix<bool,3,1> &autosize);

	shared_ptr<CGAL_Nef_polyhedron3> p3;
};

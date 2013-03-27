#ifndef POLYGON2D_H_
#define POLYGON2D_H_

#include "Geometry.h"
#include "linalg.h"
#include <vector>

typedef std::vector<Vector2d> Outline2d;

class Polygon2d : public Geometry
{
public:
	virtual size_t memsize() const;
	virtual BoundingBox getBoundingBox() const;
	virtual std::string dump() const;

	void addOutline(const Outline2d &outline);
//	class PolySet *tessellate();
private:
	std::vector<Outline2d> outlines;
};

#endif

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
	virtual unsigned int getDimension() const { return 2; }

	void addOutline(const Outline2d &outline) { this->theoutlines.push_back(outline); }
	class PolySet *tessellate() const;

	typedef std::vector<Outline2d> Outlines2d;
	const Outlines2d &outlines() const { return theoutlines; }

	void transform(const Transform2d &mat);
private:
	Outlines2d theoutlines;
};

#endif

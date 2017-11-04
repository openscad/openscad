#pragma once

#include "Geometry.h"
#include "linalg.h"
#include <vector>

/*!
   A single contour.
   positive is (optionally) used to distinguish between polygon contours and hold contours.
 */
struct Outline2d {
	Outline2d() : positive(true) {}
	std::vector<Vector2d> vertices;
	bool positive;
};

class Polygon2d : public Geometry
{
public:
	Polygon2d() : sanitized(false) {}
	virtual size_t memsize() const;
	virtual BoundingBox getBoundingBox() const;
	virtual std::string dump() const;
	virtual unsigned int getDimension() const { return 2; }
	virtual bool isEmpty() const;
	virtual Geometry *copy() const { return new Polygon2d(*this); }

	void addOutline(const Outline2d &outline) { this->theoutlines.push_back(outline); }
	class PolySet *tessellate() const;

	typedef std::vector<Outline2d> Outlines2d;
	const Outlines2d &outlines() const { return theoutlines; }

	void transform(const Transform2d &mat);
	void resize(const Vector2d &newsize, const Eigen::Matrix<bool, 2, 1> &autosize);

	bool isSanitized() const { return this->sanitized; }
	void setSanitized(bool s) { this->sanitized = s; }
	bool is_convex() const;
private:
	Outlines2d theoutlines;
	bool sanitized;
};

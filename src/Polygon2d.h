#pragma once

#include <vector>
#include "Geometry.h"
#include "linalg.h"
#include <numeric>

/*!
	A single contour.
	positive is (optionally) used to distinguish between polygon contours and hold contours.
*/
struct Outline2d {
	Outline2d() : positive(true) {}
	VectorOfVector2d vertices;
	bool positive;
	BoundingBox getBoundingBox() const;
};

class Polygon2d : public Geometry
{
public:
	VISITABLE_GEOMETRY();
	Polygon2d() : sanitized(false) {}
	size_t memsize() const override;
	BoundingBox getBoundingBox() const override;
	std::string dump() const override;
	unsigned int getDimension() const override { return 2; }
	bool isEmpty() const override;
	Geometry *copy() const override { return new Polygon2d(*this); }
	size_t numFacets() const override {
		return std::accumulate(theoutlines.begin(), theoutlines.end(), 0,
			[](size_t a, const Outline2d& b) { return a + b.vertices.size(); }
		);
	};
	void addOutline(const Outline2d &outline) { this->theoutlines.push_back(outline); }
	class PolySet *tessellate() const;

	typedef std::vector<Outline2d> Outlines2d;
	const Outlines2d &outlines() const { return theoutlines; }

	void transform(const Transform2d &mat);
	void resize(const Vector2d &newsize, const Eigen::Matrix<bool,2,1> &autosize);
	virtual void resize(const Vector3d &newsize, const Eigen::Matrix<bool,3,1> &autosize) override {
		resize(Vector2d(newsize[0], newsize[1]), Eigen::Matrix<bool,2,1>(autosize[0], autosize[1]));
	}

	bool isSanitized() const { return this->sanitized; }
	void setSanitized(bool s) { this->sanitized = s; }
	bool is_convex() const;
private:
	Outlines2d theoutlines;
	bool sanitized;
};

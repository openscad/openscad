#pragma once

#include <vector>
#include "Geometry.h"
#include "linalg.h"

/*!
	A single contour.
	positive is (optionally) used to distinguish between polygon contours and hold contours.
*/
struct Outline2d {
	Outline2d() : positive(true) {}
	VectorOfVector2d vertices;
	bool positive;
};

class Polygon2d : public Geometry
{
	enum class Transform3dState { NONE= 0, PENDING= 1, CACHED= 2 };
public:
	Polygon2d() : sanitized(false), trans3dState(Transform3dState::NONE) {}
	size_t memsize() const override;
	BoundingBox getBoundingBox() const override;
	std::string dump() const override;
	unsigned int getDimension() const override { return 2; }
	bool isEmpty() const override;
	Geometry *copy() const override { return new Polygon2d(*this); }

	void addOutline(const Outline2d &outline) {
		if (trans3dState != Transform3dState::NONE) mergeTrans3d();
		this->theoutlines.push_back(outline);
	}
	class PolySet *tessellate(bool in3d= false) const;

	typedef std::vector<Outline2d> Outlines2d;
	const Outlines2d &outlines() const { return trans3dState == Transform3dState::NONE? theoutlines : transformedOutlines(); }
	const Outlines2d &untransformedOutlines() const { return theoutlines; }
	const Outlines2d &transformedOutlines() const;

	void transform(const Transform2d &mat);
	void transform3d(const Transform3d &mat);
	bool hasTransform3d() { return trans3dState != Transform3dState::NONE; }
	const Transform3d &getTransform3d() const {
		// lazy initialization doesn't actually violate 'const'
		if (trans3dState == Transform3dState::NONE)
			const_cast<Polygon2d*>(this)->trans3d= Transform3d::Identity();
		return trans3d;
	}
	void resize(const Vector2d &newsize, const Eigen::Matrix<bool,2,1> &autosize);

	bool isSanitized() const { return this->sanitized; }
	void setSanitized(bool s) { this->sanitized = s; }
	bool is_convex() const;
private:
	Outlines2d theoutlines;
	bool sanitized;

	Transform3dState trans3dState;
	Transform3d trans3d;
	Outlines2d trans3dOutlines;
	void mergeTrans3d();
	void applyTrans3dToOutlines(Polygon2d::Outlines2d &outlines) const;
};

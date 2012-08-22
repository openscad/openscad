#ifndef LINALG_H_
#define LINALG_H_

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/Dense>

using Eigen::Vector2d;
using Eigen::Vector3d;
using Eigen::Vector3f;
typedef Eigen::AlignedBox<double, 3> BoundingBox;
using Eigen::Matrix3f;
using Eigen::Matrix3d;
using Eigen::Matrix4d;
#if EIGEN_WORLD_VERSION >= 3
#define Transform3d Eigen::Affine3d
#else
using Eigen::Transform3d;
#endif

bool matrix_contains_infinity( const Transform3d &m );
bool matrix_contains_nan( const Transform3d &m );

BoundingBox operator*(const Transform3d &m, const BoundingBox &box);

class Color4f : public Eigen::Vector4f
{
public:
	Color4f() { }
	Color4f(int r, int g, int b, int a = 255) { setRgb(r,g,b,a); }
	Color4f(float r, float g, float b, float a = 1.0f) : Eigen::Vector4f(r, g, b, a) { }

	void setRgb(int r, int g, int b, int a = 255) {
		*this << r/255.0f, g/255.0f, b/255.0f, a/255.0f;
	}

	bool isValid() const { return this->minCoeff() >= 0.0f; }
};

#endif

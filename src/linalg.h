#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/Dense>
#include <cstdint>

using Eigen::Vector2d;
using Eigen::Vector3d;
using Eigen::Vector3f;
using Eigen::Vector3i;

typedef Eigen::AlignedBox<double, 3> BoundingBox;
using Eigen::Matrix3f;
using Eigen::Matrix3d;
using Eigen::Matrix4d;
#define Transform3d Eigen::Affine3d
#define Transform2d Eigen::Affine2d

bool matrix_contains_infinity(const Transform3d &m);
bool matrix_contains_nan(const Transform3d &m);
int32_t hash_floating_point(double v);

template <typename Derived> bool is_finite(const Eigen::MatrixBase<Derived> &x) {
	return ((x - x).array() == (x - x).array()).all();
}

template <typename Derived> bool is_nan(const Eigen::MatrixBase<Derived> &x) {
	return !((x.array() == x.array())).all();
}

BoundingBox operator*(const Transform3d &m, const BoundingBox &box);

class Color4f : public Eigen::Vector4f
{
public:
	Color4f() { }
	Color4f(int r, int g, int b, int a = 255) { setRgb(r, g, b, a); }
	Color4f(float r, float g, float b, float a = 1.0f) : Eigen::Vector4f(r, g, b, a) { }

	void setRgb(int r, int g, int b, int a = 255) {
		*this << r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f;
	}

	bool isValid() const { return this->minCoeff() >= 0.0f; }
};

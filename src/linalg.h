#ifndef LINALG_H_
#define LINALG_H_

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/Dense>

using Eigen::Vector2d;
using Eigen::Vector3d;
typedef Eigen::AlignedBox<double, 3> BoundingBox;
using Eigen::Matrix3f;
using Eigen::Matrix3d;
using Eigen::Matrix4d;
using Eigen::Transform3d;

BoundingBox operator*(const Transform3d &m, const BoundingBox &box);

class Color4f : public Eigen::Vector4f
{
public:
	bool isValid() const { return this->minCoeff() >= 0.0f; }
};

#endif

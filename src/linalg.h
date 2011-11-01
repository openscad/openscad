#ifndef LINALG_H_
#define LINALG_H_
#ifndef EIGEN_DONT_ALIGN
#define EIGEN_DONT_ALIGN
#endif
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/Dense>

using Eigen::Vector2d;
using Eigen::Vector3d;
typedef Eigen::AlignedBox<double, 3> BoundingBox;
using Eigen::Matrix3f;
using Eigen::Matrix3d;
using Eigen::Transform3d;

#endif

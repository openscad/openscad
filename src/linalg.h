#ifndef LINALG_H_
#define LINALG_H_

#ifndef __APPLE__
#define EIGEN_DONT_VECTORIZE 1
#define EIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT 1
#endif

#include <Eigen/Core>
#include <Eigen/Geometry>

using Eigen::Vector3d;
typedef Eigen::AlignedBox<double, 3> BoundingBox;
using Eigen::Matrix3f;
using Eigen::Matrix3d;
using Eigen::Transform3d;

#endif

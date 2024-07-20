#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/Dense>
#include <cstdint>
#include <vector>

using Eigen::Vector2d;
using Eigen::Vector3d;
using Eigen::Vector3f;
using Eigen::Vector3i;

#ifdef _MSC_VER
  #include <Eigen/StdVector> // https://eigen.tuxfamily.org/dox/group__TopicStlContainers.html
  #if !EIGEN_HAS_CXX11_CONTAINERS
    #warning "Eigen has detected no support for CXX11 containers and has redefined std::vector"
  #endif
using VectorOfVector2d = std::vector<Vector2d, Eigen::aligned_allocator<Vector2d>>;
#else
using VectorOfVector2d = std::vector<Vector2d>;
#endif

using BoundingBox = Eigen::AlignedBox<double, 3>;
using Eigen::Matrix3f;
using Eigen::Matrix3d;
using Eigen::Matrix4d;
#define Transform3d Eigen::Affine3d
#define Transform2d Eigen::Affine2d

bool matrix_contains_infinity(const Transform3d& m);
bool matrix_contains_nan(const Transform3d& m);
int32_t hash_floating_point(double v);

template <typename Derived> bool is_finite(const Eigen::MatrixBase<Derived>& x) {
  //infinity minus infinity is NaN, which never compares equal to itself
  return ( (x - x).array() == (x - x).array()).all(); // NOLINT(misc-redundant-expression)
}

template <typename Derived> bool is_nan(const Eigen::MatrixBase<Derived>& x) {
  return !((x.array() == x.array())).all();
}

BoundingBox operator*(const Transform3d& m, const BoundingBox& box);

// Vector4f is fixed-size vectorizable
// Use Eigen::DontAlign so we can store Color4f in STL containers
// https://eigen.tuxfamily.org/dox/group__DenseMatrixManipulation__Alignement.html
class Color4f : public Eigen::Matrix<float, 4, 1, Eigen::DontAlign>
{
public:
  Color4f(int r, int g, int b, int a = 255) { setRgb(r, g, b, a); }
  Color4f(float r = -1.0f, float g = -1.0f, float b = -1.0f, float a = 1.0f) : Eigen::Matrix<float, 4, 1, Eigen::DontAlign>(r, g, b, a) { }

  void setRgb(int r, int g, int b, int a = 255) {
    *this << static_cast<float>(r) / 255.0f,
             static_cast<float>(g) / 255.0f,
             static_cast<float>(b) / 255.0f,
             static_cast<float>(a) / 255.0f;
  }

  [[nodiscard]] bool isValid() const { return this->minCoeff() >= 0.0f; }

  bool operator<(const Color4f &b) const {
    for (int i = 0; i < 4; i++) {
      if ((*this)[i] < b[i]) return true;
      if ((*this)[i] > b[i]) return false;
    }
    return false;
  }
};

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
using Eigen::Vector4d;
using Eigen::Vector4f;

#ifdef _MSC_VER
#include <Eigen/StdVector>  // https://eigen.tuxfamily.org/dox/group__TopicStlContainers.html
#if !EIGEN_HAS_CXX11_CONTAINERS
#warning "Eigen has detected no support for CXX11 containers and has redefined std::vector"
#endif
using VectorOfVector2d = std::vector<Vector2d, Eigen::aligned_allocator<Vector2d>>;
#else
using VectorOfVector2d = std::vector<Vector2d>;
#endif

using BoundingBox = Eigen::AlignedBox<double, 3>;
using Eigen::Matrix3d;
using Eigen::Matrix3f;
using Eigen::Matrix4d;
#define Transform3d Eigen::Affine3d
#define Transform2d Eigen::Affine2d

bool matrix_contains_infinity(const Transform3d& m);
bool matrix_contains_nan(const Transform3d& m);
int32_t hash_floating_point(double v);

template <typename Derived>
bool is_finite(const Eigen::MatrixBase<Derived>& x)
{
  // infinity minus infinity is NaN, which never compares equal to itself
  return ((x - x).array() == (x - x).array()).all();  // NOLINT(misc-redundant-expression)
}

template <typename Derived>
bool is_nan(const Eigen::MatrixBase<Derived>& x)
{
  return !((x.array() == x.array())).all();
}

BoundingBox operator*(const Transform3d& m, const BoundingBox& box);

class Color4f
{
public:
  Color4f(const Vector4f& v) : color_(v) {}
  Color4f(int r, int g, int b, int a = 255) { setRgba(r, g, b, a); }
  Color4f(float r = -1.0f, float g = -1.0f, float b = -1.0f, float a = -1.0f) : color_(r, g, b, a) {}

  [[nodiscard]] bool isValid() const { return color_.minCoeff() >= 0.0f; }
  [[nodiscard]] bool hasRgb() const
  {
    return color_[0] >= 0.0f && color_[1] >= 0.0f && color_[2] >= 0.0f;
  }
  [[nodiscard]] bool hasAlpha() const { return color_[3] >= 0.0f; }

  void setRgba(int r, int g, int b, int a = 255)
  {
    color_ << static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f,
      static_cast<float>(b) / 255.0f, static_cast<float>(a) / 255.0f;
  }
  void setRgba(float r, float g, float b, float a = 1.0f) { color_ << r, g, b, a; }
  void setRgb(float r, float g, float b) { color_.head<3>() << r, g, b; }
  void setAlpha(float a) { color_[3] = a; }

  bool getRgba(int& r, int& g, int& b, int& a) const
  {
    if (!isValid()) return false;
    r = std::clamp(static_cast<int>(this->r() * 255.0f), 0, 255);
    g = std::clamp(static_cast<int>(this->g() * 255.0f), 0, 255);
    b = std::clamp(static_cast<int>(this->b() * 255.0f), 0, 255);
    a = std::clamp(static_cast<int>(this->a() * 255.0f), 0, 255);
    return true;
  }

  bool getRgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const
  {
    if (!isValid()) return false;
    r = static_cast<uint8_t>(std::clamp(this->r(), 0.0f, 1.0f) * 255.0f);
    g = static_cast<uint8_t>(std::clamp(this->g(), 0.0f, 1.0f) * 255.0f);
    b = static_cast<uint8_t>(std::clamp(this->b(), 0.0f, 1.0f) * 255.0f);
    a = static_cast<uint8_t>(std::clamp(this->a(), 0.0f, 1.0f) * 255.0f);
    return true;
  }

  bool getRgba(float& r, float& g, float& b, float& a) const
  {
    if (!isValid()) return false;
    r = this->r();
    g = this->g();
    b = this->b();
    a = this->a();
    return true;
  }

  [[nodiscard]] Vector4f toVector4f() const { return color_; }

  [[nodiscard]] float r() const { return color_[0]; }
  [[nodiscard]] float g() const { return color_[1]; }
  [[nodiscard]] float b() const { return color_[2]; }
  [[nodiscard]] float a() const { return color_[3]; }

  [[nodiscard]] bool operator<(const Color4f& b) const
  {
    for (int i = 0; i < 4; i++) {
      if (color_[i] < b.color_[i]) return true;
      if (color_[i] > b.color_[i]) return false;
    }
    return false;
  }

  [[nodiscard]] bool operator==(const Color4f& b) const { return color_ == b.color_; }

  [[nodiscard]] bool operator!=(const Color4f& b) const { return !(*this == b); }

  [[nodiscard]] size_t hash() const
  {
    size_t hash = 0;
    // Gcc version 10.2.1 (Debian 11) fails to handle the
    // range-for loop, it can't find the begin() definition
    // of the Eigen::Matrix (with Eigen 3.3.9).
    hash = std::hash<float>{}(r()) ^ (hash << 1);
    hash = std::hash<float>{}(g()) ^ (hash << 1);
    hash = std::hash<float>{}(b()) ^ (hash << 1);
    hash = std::hash<float>{}(a()) ^ (hash << 1);
    return hash;
  }

private:
  // Vector4f is fixed-size vectorizable
  // Use Eigen::DontAlign so we can store Color4f in STL containers
  // https://eigen.tuxfamily.org/dox/group__DenseMatrixManipulation__Alignement.html
  Eigen::Matrix<float, 4, 1, Eigen::DontAlign> color_;
};

template <>
struct std::hash<Color4f> {
  std::size_t operator()(Color4f const& c) const noexcept { return c.hash(); }
};

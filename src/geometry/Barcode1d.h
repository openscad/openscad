#pragma once

#include <utility>
#include <memory>
#include <cstddef>
#include <string>
#include <vector>
#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include <numeric>

/*!
   A single contour.
   positive is (optionally) used to distinguish between polygon contours and hole contours.
 */
struct Edge1d {
  Edge1d() = default;
  Edge1d(double begin, double end)
  {
    this->begin = begin;
    this->end = end;
  }
  double begin, end;
  Color4f color;
  bool positive{true};
  [[nodiscard]] BoundingBox getBoundingBox() const;
};

class Barcode1d : public Geometry
{
  enum class Transform3dState { NONE = 0, PENDING = 1, CACHED = 2 };

public:
  VISITABLE_GEOMETRY();
  Barcode1d() = default;
  Barcode1d(Edge1d edge);
  [[nodiscard]] size_t memsize() const override;
  [[nodiscard]] BoundingBox getBoundingBox() const override;
  [[nodiscard]] std::string dump() const override;
  [[nodiscard]] unsigned int getDimension() const override { return 1; }
  [[nodiscard]] bool isEmpty() const override;
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override;
  [[nodiscard]] size_t numFacets() const override { return 1; }
  void addEdge(const Edge1d& edge)
  {
    if (trans3dState != Transform3dState::NONE) mergeTrans3d();
    this->theedges.push_back(edge);
  }
  [[nodiscard]] std::unique_ptr<PolySet> tessellate(bool in3d = false) const;
  [[nodiscard]] double area() const;
  void setColor(const Color4f& c) override;

  using Edges1d = std::vector<Edge1d>;
  //// Note: The "using" here is a kludge to avoid a compiler warning.
  //// It would be better to fix the class relationships, so that Barcode1d does
  //// not inherit an unused 3d transform function.
  //// But that will likely require significant refactoring.
  const Edges1d& edges() const
  {
    return trans3dState == Transform3dState::NONE ? theedges : transformedEdges();
  }
  const Edges1d& untransformedEdges() const { return theedges; }
  const Edges1d& transformedEdges() const;
  using Geometry::transform;

  void transform(const Transform2d& mat);
  void transform3d(const Transform3d& mat);
  bool hasTransform3d() const { return trans3dState != Transform3dState::NONE; }
  std::shared_ptr<Polygon2d> to2d(void) const;
  const Transform3d& getTransform3d() const
  {
    // lazy initialization doesn't actually violate 'const'
    if (trans3dState == Transform3dState::NONE)
      const_cast<Barcode1d *>(this)->trans3d = Transform3d::Identity();
    return trans3d;
  }
  void resize(const Vector2d& newsize, const Eigen::Matrix<bool, 2, 1>& autosize);
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override
  {
    resize(Vector2d(newsize[0], newsize[1]), Eigen::Matrix<bool, 2, 1>(autosize[0], autosize[1]));
  }

  [[nodiscard]] bool isSanitized() const { return this->sanitized; }
  void setSanitized(bool s) { this->sanitized = s; }
  [[nodiscard]] bool is_convex() const;

private:
  Edges1d theedges;
  bool sanitized{false};
  Transform3dState trans3dState{Transform3dState::NONE};
  Transform3d trans3d;
  Edges1d trans3dEdges;
  void mergeTrans3d();
  void applyTrans3dToEdges(Barcode1d::Edges1d& edges) const;
};

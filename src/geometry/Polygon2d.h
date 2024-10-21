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
struct Outline2d {
  Outline2d() = default;
  VectorOfVector2d vertices;
  bool positive{true};
  [[nodiscard]] BoundingBox getBoundingBox() const;
};

class Polygon2d : public Geometry
{
public:
  VISITABLE_GEOMETRY();
  Polygon2d() = default;
  Polygon2d(Outline2d outline);
  [[nodiscard]] size_t memsize() const override;
  [[nodiscard]] BoundingBox getBoundingBox() const override;
  [[nodiscard]] std::string dump() const override;
  [[nodiscard]] unsigned int getDimension() const override { return 2; }
  [[nodiscard]] bool isEmpty() const override;
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override;
  [[nodiscard]] size_t numFacets() const override {
    return std::accumulate(theoutlines.begin(), theoutlines.end(), 0,
                           [](size_t a, const Outline2d& b) {
      return a + b.vertices.size();
    }
                           );
  }
  void addOutline(Outline2d outline) { this->theoutlines.push_back(std::move(outline)); }
  [[nodiscard]] std::unique_ptr<PolySet> tessellate() const;
  [[nodiscard]] double area() const;

  using Outlines2d = std::vector<Outline2d>;
  [[nodiscard]] const Outlines2d& outlines() const { return theoutlines; }
  // Note: The "using" here is a kludge to avoid a compiler warning.
  // It would be better to fix the class relationships, so that Polygon2d does
  // not inherit an unused 3d transform function.
  // But that will likely require significant refactoring.
  using Geometry::transform;

  void transform(const Transform2d& mat);
  void resize(const Vector2d& newsize, const Eigen::Matrix<bool, 2, 1>& autosize);
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override {
    resize(Vector2d(newsize[0], newsize[1]), Eigen::Matrix<bool, 2, 1>(autosize[0], autosize[1]));
  }

  [[nodiscard]] bool isSanitized() const { return this->sanitized; }
  void setSanitized(bool s) { this->sanitized = s; }
  [[nodiscard]] bool is_convex() const;
private:
  Outlines2d theoutlines;
  bool sanitized{false};
};

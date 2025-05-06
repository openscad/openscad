#pragma once

#include "geometry/cgal/cgal.h"
#include "geometry/Geometry.h"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include "geometry/linalg.h"

class CGALNefGeometry : public Geometry
{
public:
  VISITABLE_GEOMETRY();
  CGALNefGeometry() = default;
  CGALNefGeometry(std::shared_ptr<const CGAL_Nef_polyhedron3> p) : p3(std::move(p)) {}
  CGALNefGeometry(const CGALNefGeometry& src);
  CGALNefGeometry& operator=(const CGALNefGeometry&) = default;
  CGALNefGeometry(CGALNefGeometry&&) = default;
  CGALNefGeometry& operator=(CGALNefGeometry&&) = default;
  ~CGALNefGeometry() override = default;

  [[nodiscard]] size_t memsize() const override;
  // FIXME: Implement, but we probably want a high-resolution BBox..
  [[nodiscard]] BoundingBox getBoundingBox() const override;
  [[nodiscard]] std::string dump() const override;
  [[nodiscard]] unsigned int getDimension() const override { return 3; }
  // Empty means it is a geometric node which has zero area/volume
  [[nodiscard]] bool isEmpty() const override;
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override;
  [[nodiscard]] size_t numFacets() const override { return p3->number_of_facets(); }

  void reset() { p3.reset(); }
  CGALNefGeometry operator+(const CGALNefGeometry& other) const;
  CGALNefGeometry& operator+=(const CGALNefGeometry& other);
  CGALNefGeometry& operator*=(const CGALNefGeometry& other);
  CGALNefGeometry& operator-=(const CGALNefGeometry& other);
  CGALNefGeometry& minkowski(const CGALNefGeometry& other);
  void transform(const Transform3d& matrix) override;
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override;

  std::shared_ptr<const CGAL_Nef_polyhedron3> p3;
};

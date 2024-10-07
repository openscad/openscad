#pragma once

#include "geometry/cgal/cgal.h"
#include "geometry/Geometry.h"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include "geometry/linalg.h"

class CGAL_Nef_polyhedron : public Geometry
{
public:
  VISITABLE_GEOMETRY();
  CGAL_Nef_polyhedron() = default;
  CGAL_Nef_polyhedron(std::shared_ptr<const CGAL_Nef_polyhedron3> p) : p3(std::move(p)) {}
  CGAL_Nef_polyhedron(const CGAL_Nef_polyhedron& src);
  CGAL_Nef_polyhedron& operator=(const CGAL_Nef_polyhedron&) = default;
  CGAL_Nef_polyhedron(CGAL_Nef_polyhedron&&) = default;
  CGAL_Nef_polyhedron& operator=(CGAL_Nef_polyhedron&&) = default;
  ~CGAL_Nef_polyhedron() override = default;

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
  CGAL_Nef_polyhedron operator+(const CGAL_Nef_polyhedron& other) const;
  CGAL_Nef_polyhedron& operator+=(const CGAL_Nef_polyhedron& other);
  CGAL_Nef_polyhedron& operator*=(const CGAL_Nef_polyhedron& other);
  CGAL_Nef_polyhedron& operator-=(const CGAL_Nef_polyhedron& other);
  CGAL_Nef_polyhedron& minkowski(const CGAL_Nef_polyhedron& other);
  void transform(const Transform3d& matrix) override;
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override;

  std::shared_ptr<const CGAL_Nef_polyhedron3> p3;
};

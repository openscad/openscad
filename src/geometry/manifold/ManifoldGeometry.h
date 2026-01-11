// Portions of this file are Copyright 2023 Google LLC, and licensed under GPL2+. See COPYING.
#pragma once

#include "geometry/Geometry.h"
#include <functional>
#include <cstdint>
#include <memory>
#include "geometry/linalg.h"
#include <manifold/manifold.h>
#include <cstddef>
#include <map>
#include <set>
#include <string>

namespace manifold {
class Manifold;
};

/*! A mutable polyhedron backed by a manifold::Manifold
 */
class ManifoldGeometry : public Geometry
{
public:
  VISITABLE_GEOMETRY();

  ManifoldGeometry();
  ManifoldGeometry(manifold::Manifold object, const std::set<uint32_t>& originalIDs = {},
                   const std::map<uint32_t, Color4f>& originalIDToColor = {},
                   const std::set<uint32_t>& subtractedIDs = {});
  ManifoldGeometry(const ManifoldGeometry& other) = default;

  [[nodiscard]] bool isEmpty() const override;
  [[nodiscard]] size_t numFacets() const override;
  [[nodiscard]] size_t numVertices() const;
  [[nodiscard]] bool isManifold() const;
  [[nodiscard]] bool isValid() const;
  void clear();

  [[nodiscard]] size_t memsize() const override;
  [[nodiscard]] BoundingBox getBoundingBox() const override;

  [[nodiscard]] std::string dump() const override;
  [[nodiscard]] unsigned int getDimension() const override { return 3; }
  [[nodiscard]] std::unique_ptr<Geometry> copy() const override;

  [[nodiscard]] std::shared_ptr<PolySet> toPolySet() const;

  template <class Polyhedron>
  [[nodiscard]] std::shared_ptr<Polyhedron> toPolyhedron() const;

  /*! union. */
  ManifoldGeometry operator+(const ManifoldGeometry& other) const;
  /*! intersection. */
  ManifoldGeometry operator*(const ManifoldGeometry& other) const;
  /*! difference. */
  ManifoldGeometry operator-(const ManifoldGeometry& other) const;
  /*! minkowksi operation. */
  ManifoldGeometry minkowski(const ManifoldGeometry& other) const;

  Polygon2d slice() const;
  Polygon2d project() const;

  void transform(const Transform3d& mat) override;
  void setColor(const Color4f& c) override;
  void toOriginal();
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override;

  /*! Iterate over all vertices' points until the function returns true (for done). */
  void foreachVertexUntilTrue(const std::function<bool(const manifold::vec3& pt)>& f) const;

  const manifold::Manifold& getManifold() const;

private:
  ManifoldGeometry binOp(const ManifoldGeometry& lhs, const ManifoldGeometry& rhs,
                         manifold::OpType opType) const;
  void updateCachedSize() const;

  manifold::Manifold manifold_;
  std::set<uint32_t> originalIDs_;
  std::map<uint32_t, Color4f> originalIDToColor_;
  std::set<uint32_t> subtractedIDs_;
  mutable size_t cached_size_{0};
};

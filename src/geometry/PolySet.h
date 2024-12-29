#pragma once

#include "geometry/Geometry.h"
#include "geometry/linalg.h"
#include "geometry/GeometryUtils.h"
#include "geometry/Polygon2d.h"
#include "utils/boost-utils.h"

#include <cstdint>
#include <memory>
#include <cstddef>
#include <string>
#include <vector>

class PolySetBuilder;

class PolySet : public Geometry
{
  friend class PolySetBuilder;	
public:
  VISITABLE_GEOMETRY();
  PolygonIndices indices;
  std::vector<Vector3d> vertices;
  // Per polygon color, indexing the colors vector below. Can be empty, and -1 means no specific color.
  std::vector<int32_t> color_indices; 
  std::vector<Color4f> colors;

  PolySet(unsigned int dim, boost::tribool convex = unknown);

  size_t memsize() const override;
  BoundingBox getBoundingBox() const override;
  std::string dump() const override;
  unsigned int getDimension() const override { return dim_; }
  bool isEmpty() const override { return indices.empty(); }
  std::unique_ptr<Geometry> copy() const override;

  void quantizeVertices(std::vector<Vector3d> *pPointsOut = nullptr);
  size_t numFacets() const override { return indices.size(); }
  void transform(const Transform3d& mat) override;
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override;
  void setColor(const Color4f& c) override;

  bool isConvex() const;
  boost::tribool convexValue() const { return convex_; }

  bool isTriangular() const { return is_triangular_; }
  void setTriangular(bool triangular) { is_triangular_ = triangular; }

  static std::unique_ptr<PolySet> createEmpty() { return std::make_unique<PolySet>(3); }

private:
  bool is_triangular_ = false;
  unsigned int dim_;
  mutable boost::tribool convex_;
  mutable BoundingBox bbox_;
};

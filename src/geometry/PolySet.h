#pragma once

#include "Geometry.h"
#include "linalg.h"
#include "GeometryUtils.h"
#include "Polygon2d.h"
#include "boost-utils.h"

#include <vector>
#include <string>
class PolySetBuilder;

class PolySet : public Geometry
{
  friend class PolySetBuilder;	
public:
  VISITABLE_GEOMETRY();
  PolygonIndices indices;
  std::vector<Vector3d> vertices;

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

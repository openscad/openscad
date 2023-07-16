#pragma once

#include "Geometry.h"
#include "linalg.h"
#include "GeometryUtils.h"
#include "Polygon2d.h"
#include "boost-utils.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <hash.h>

class PolySetBuilder;

class PolySet : public Geometry
{
  friend class PolySetBuilder;	
public:
  VISITABLE_GEOMETRY();
  PolygonIndices indices;
  std::vector<Vector3d> vertices;
  std::unordered_map<Vector3d, int> pointMap;
  int pointIndex(const Vector3d &pt);
  int pointIndex(const Vector3f &pt);

  PolySet(unsigned int dim, boost::tribool convex = unknown);
  PolySet(Polygon2d origin);

  const Polygon2d& getPolygon() const { return polygon; }

  size_t memsize() const override;
  BoundingBox getBoundingBox() const override;
  std::string dump() const override;
  unsigned int getDimension() const override { return this->dim; }
  bool isEmpty() const override { return indices.size() == 0; }
  Geometry *copy() const override { return new PolySet(*this); }

  void quantizeVertices(std::vector<Vector3d> *pPointsOut = nullptr);
  size_t numFacets() const override { return indices.size(); }
  void reserve(size_t numFacets) { indices.reserve(numFacets); }
  int append_coord(const Vector3d &coord);
  void append_poly(size_t expected_vertex_count);
  void append_poly(const std::vector<int> &inds);
  void append_vertex(int ind);
  void insert_vertex(int ind); 
  void append(const PolySet& ps);

  void transform(const Transform3d& mat) override;
  void resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize) override;

  bool is_convex() const;
  boost::tribool convexValue() const { return this->convex; }

private:
  Polygon2d polygon;
  unsigned int dim;
  mutable boost::tribool convex;
  mutable BoundingBox bbox;
  mutable bool dirty;
};

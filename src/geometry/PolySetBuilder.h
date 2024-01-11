#pragma once

#include <memory>

#include <Reindexer.h>
#include "Polygon2d.h"
#include "boost-utils.h"
#include "GeometryUtils.h"

class PolySet;

class PolySetBuilder
{
public:
  PolySetBuilder(int vertices_count = 0, int indices_count = 0, int dim = 3, boost::tribool convex = unknown);
  PolySetBuilder(const Polygon2d& polygon2d);
  void setConvexity(int n);
  int vertexIndex(const Vector3d& coord);
  int numVertices() const;
  void appendPoly(int nvertices);
  void append(const PolySet &ps);
  void appendPoly(const std::vector<int>& inds);
  void appendGeometry(const std::shared_ptr<const Geometry>& geom);
  void appendPoly(const std::vector<Vector3d>& v);
  void appendVertex(int n);
  void appendVertex(const Vector3d &v);
  void prependVertex(int n);
  void prependVertex(const Vector3d &v);
  std::unique_ptr<PolySet> build();
private:
  Reindexer<Vector3d> vertices_;
  PolygonIndices indices_;
  Polygon2d polygon2d_;
  int convexity_{1};
  int dim_;
  boost::tribool convex_;
};

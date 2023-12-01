#pragma once

#include <Reindexer.h>
#include "Polygon2d.h"
#include <memory>
#include "boost-utils.h"
class PolySet;

class PolySetBuilder
{
public:
  PolySetBuilder(int vertices_count=0, int indices_count=0, int dim=3, boost::tribool convex=unknown);
  PolySetBuilder(const Polygon2d &pol);
  void setConvexity(int n);
  int vertexIndex(const Vector3d &coord);
  int numVertices();
  void appendPoly(int nvertices);
  void append(const PolySet *ps);
  void appendPoly(const std::vector<int> &inds);
  void appendGeometry(const shared_ptr<const Geometry>& geom);
  void appendPoly(const std::vector<Vector3d> &v);
  void appendVertex(int n);
  void prependVertex(int n);
  PolySet *build();
private:  
  PolySet *ps;
  Reindexer<Vector3d> allVertices;
};

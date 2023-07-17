#pragma once

#include <Reindexer.h>
#include <memory>
class PolySet;

class PolySetBuilder
{
  std::shared_ptr<PolySet> ps;
  Reindexer<Vector3d> allVertices;
public:
  PolySetBuilder(int vertices_count=0, int indices_count=0, int dim=3, bool convex=true);
  void reset(void);
  void setConvexity(int n);
  int vertexIndex(const Vector3d &coord);
  int numVertices(void);
  void append_poly(int nvertices);
  void append(PolySet *ps);
  void append_poly(const std::vector<int> &inds);
  void append_poly(const std::vector<Vector3d> &v);
  void append_vertex(int n);
  void prepend_vertex(int n);
  std::shared_ptr<PolySet> result(void);
};

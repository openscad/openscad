#pragma once

#include <Reindexer.h>
class PolySet;

class PolySetBuilder
{
  PolySet *ps;
  Reindexer<Vector3d> allVertices;
public:
  PolySetBuilder(int vertices_count, int indices_count, int dim=3, bool convex=true);
  int vertexIndex(const Vector3d &coord);
  void append_poly(const std::vector<int> &inds);
  void append_poly(const std::vector<Vector3d> &v);
  PolySet *result(void);
};

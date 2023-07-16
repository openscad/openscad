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
class PolySet;

class PolySetBuilder
{
  PolySet *ps;
  std::unordered_map<Vector3d, int> pointMap;
public:
  PolySetBuilder(int dim,int vertices_count);
  int vertexIndex(const Vector3d &coord);
  void append_poly(const std::vector<int> &inds);
  PolySet *result(void);
};

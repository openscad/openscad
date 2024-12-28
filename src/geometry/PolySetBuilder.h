#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "geometry/Reindexer.h"
#include "geometry/Polygon2d.h"
#include "utils/boost-utils.h"
#include "geometry/GeometryUtils.h"

class PolySet;

class PolySetBuilder
{
public:
  PolySetBuilder(int vertices_count = 0, int indices_count = 0, int dim = 3, boost::tribool convex = unknown);
  void reserve(int vertices_count = 0, int indices_count = 0);
  void setConvexity(int n);
  int vertexIndex(const Vector3d& coord);
  int numVertices() const;
  int numPolygons() const;

  void appendPolySet(const PolySet &ps);
  void appendGeometry(const std::shared_ptr<const Geometry>& geom);
  void appendPolygon(const std::vector<int>& inds);
  void appendPolygon(const std::vector<Vector3d>& v, const Color4f & color = {});

  void beginPolygon(int nvertices, const Color4f & color = {});
  void addVertex(int ind);
  void addVertex(const Vector3d &v);
  // Calling this is optional; will be called automatically when adding a new polygon or building the PolySet
  void endPolygon();
  bool empty() const {
    return indices_.empty();
  }

  std::unique_ptr<PolySet> build();
private:
  Reindexer<Vector3d> vertices_;
  PolygonIndices indices_;
  std::vector<int32_t> color_indices_;
  std::vector<Color4f> colors_;
  std::map<Color4f, int32_t> color_map_;
  int convexity_{1};
  int dim_;
  boost::tribool convex_;

  // Will be initialized by beginPolygon() and cleared by endPolygon()
  IndexedFace current_polygon_;
};

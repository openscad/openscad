#pragma once

#include "linalg.h"
#include <vector>

class MarkedVector3d : public Vector3d {
public:
  bool marked;
}
typedef std::vector<Vector3d> Polygon;
typedef std::vector<Polygon> Polygons;

using IndexedFace = std::vector<int>;
using IndexedTriangle = Vector3i;

struct IndexedPolygons {
  std::vector<Vector3f> vertices;
  std::vector<IndexedFace> faces;
};

struct IndexedTriangleMesh {
  std::vector<Vector3f> vertices;
  std::vector<IndexedTriangle> triangles;
};

// Indexed polygon mesh, where each polygon can have holes
struct IndexedPolyMesh {
  std::vector<Vector3f> vertices;
  std::vector<std::vector<IndexedFace>> polygons;
};

namespace GeometryUtils {
bool tessellatePolygon(const Polygon& polygon,
                       Polygons& triangles,
                       const Vector3f *normal = nullptr);
bool tessellatePolygonWithHoles(const std::vector<Vector3f>& vertices,
                                const std::vector<IndexedFace>& faces,
                                std::vector<IndexedTriangle>& triangles,
                                const Vector3f *normal = nullptr);

int findUnconnectedEdges(const std::vector<std::vector<IndexedFace>>& polygons);
int findUnconnectedEdges(const std::vector<IndexedTriangle>& triangles);

Transform3d getResizeTransform(const BoundingBox &bbox, const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize);
}

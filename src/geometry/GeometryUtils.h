#pragma once

#include "geometry/linalg.h"
#include "geometry/Geometry.h"
#include <vector>
#include <boost/container/small_vector.hpp>
#include <memory>

using Polygon = std::vector<Vector3d>;
using Polygons = std::vector<Polygon>;

// faces are usually triangles or quads
using IndexedFace = boost::container::small_vector<int, 4>;
using IndexedTriangle = Vector3i;
using PolygonIndices = std::vector<IndexedFace>;

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
std::shared_ptr<const Geometry> getBackendSpecificGeometry(const std::shared_ptr<const Geometry>& geom);

}

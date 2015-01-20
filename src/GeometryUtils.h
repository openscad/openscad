#pragma once

#include "linalg.h"
#include <vector>

typedef std::vector<Vector3d> Polygon;
typedef std::vector<Polygon> Polygons;

typedef std::vector<int> IndexedFace;
typedef Vector3i IndexedTriangle;

struct IndexedPolygons {
	std::vector<Vector3f> vertices;
	std::vector<IndexedFace> faces;
};

struct IndexedTriangleMesh {
	std::vector<Vector3f> vertices;
	std::vector<IndexedTriangle> triangles;
};

namespace GeometryUtils {
	bool tessellatePolygon(const Polygon &polygon, Polygons &triangles,
												 const Vector3f *normal = NULL);
	bool tessellatePolygonWithHoles(const IndexedPolygons &polygons, std::vector<IndexedTriangle> &triangles,
																	const Vector3f *normal = NULL);
}

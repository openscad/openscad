#pragma once

#include "linalg.h"
#include <vector>
#include <memory>

typedef std::vector<Vector3d> Polygon;
typedef std::vector<Polygon> Polygons;


typedef std::vector<size_t> IndexedFace;
typedef std::vector<IndexedFace> IndexedPolygon;
typedef std::vector<IndexedPolygon> IndexedPolygons;

typedef Vector3i IndexedTriangle;
typedef std::vector<IndexedTriangle> IndexedTriangles;

namespace GeometryUtils {
	bool tessellatePolygonWithHoles(const std::vector<Vector3f>& vertices,
					const std::vector<IndexedFace> &faces, 
					std::vector<IndexedTriangle> &triangles,
					const Vector3f *normal = nullptr, bool clean_faces = true);
	
	int findUnconnectedEdges(const std::vector<std::vector<IndexedFace>> &polygons);
	int findUnconnectedEdges(const std::vector<IndexedTriangle> &triangles);
}

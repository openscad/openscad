#pragma once

#include "linalg.h"
#include <vector>
#include "AST.h"

typedef std::vector<Vector3d> Polygon;
typedef std::vector<Polygon> Polygons;

typedef std::vector<int> IndexedFace;
typedef Vector3i IndexedTriangle;

typedef std::pair<int,int> IndexedEdge;

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
	bool tessellatePolygon(const Polygon &polygon,
												 Polygons &triangles,
												 const Vector3f *normal = nullptr);
	bool tessellatePolygonWithHoles(const std::vector<Vector3f>& vertices,
																	const std::vector<IndexedFace> &faces, 
																	std::vector<IndexedTriangle> &triangles,
																	const Vector3f *normal = nullptr,
																	const Location &loc = Location::NONE,
																	const std::string &docPath="");

	int findUnconnectedEdges(const std::vector<std::vector<IndexedFace>> &polygons);
	int findUnconnectedEdges(const std::vector<IndexedTriangle> &triangles);
	std::vector<IndexedEdge> reportUnconnectedEdges(const std::vector<std::vector<IndexedFace>> &polygons);
}

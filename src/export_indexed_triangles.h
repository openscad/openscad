#pragma once

#include <vector>
#include <utility>

#include "GeometryUtils.h"
#include "linalg.h"
#include "polyset.h"


std::pair<std::vector<Vector3d>, std::vector<IndexedTriangle>>
export_indexed_triangles(const PolySet &ps);

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"

std::pair<std::vector<Vector3d>, std::vector<IndexedTriangle>>
export_indexed_triangles(const CGAL_Polyhedron &P);

#endif // ENABLE_CGAL

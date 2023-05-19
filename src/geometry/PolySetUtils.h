#pragma once

class Polygon2d;
class PolySet;
class IndexedTriangleMesh;

namespace PolySetUtils {

Polygon2d *project(const PolySet& ps);
void tessellate_faces(const PolySet& inps, PolySet& outps);
void tessellate_faces(const PolySet& inps, IndexedTriangleMesh& outps);
bool is_approximately_convex(const PolySet& ps);

}

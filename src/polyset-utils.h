#ifndef POLYSET_UTILS_H_
#define POLYSET_UTILS_H_

#include <polyset.h>

typedef PolySet::Polygon Polygon3d;

namespace PolysetUtils {

	Polygon2d *project(const PolySet &ps);
	void tessellate_faces(const PolySet &inps, PolySet &outps);
	bool triangulate_polygon(const Polygon3d &pgon, std::vector<Polygon3d> &triangles);
	bool is_simple(Polygon3d const&);
	bool is_convex(Polygon3d const&);
};

#endif

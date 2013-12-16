#ifndef POLYSET_UTILS_H_
#define POLYSET_UTILS_H_

class Polygon2d;
class PolySet;

namespace PolysetUtils {

	const Polygon2d *project(const PolySet &ps);
	void tessellate_faces(const PolySet &inps, PolySet &outps);

};

#endif

#pragma once

class Polygon2d;
class PolySet;

namespace PolysetUtils {

	Polygon2d *project(const PolySet &ps);
	void tessellate_faces(const PolySet &inps, PolySet &outps);

};

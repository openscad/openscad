#pragma once

class Polygon2d;
class PolySet;

namespace PolysetUtils {

	Polygon2d *project(const PolySet &ps);
	void tessellate_faces(const PolySet &inps, PolySet &outps);
	bool is_approximately_convex(const PolySet &ps);

	// for probe()
	void computeVolume(const PolySet &ps,double &volumeTotal,double centerOfMass[3]);

};

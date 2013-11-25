#include "polyset-utils.h"
#include "polyset.h"
#include "Polygon2d.h"

#include <boost/foreach.hpp>

namespace PolysetUtils {

	const Polygon2d *project(const PolySet &ps) {
		Polygon2d *poly = new Polygon2d;

		BOOST_FOREACH(const PolySet::Polygon &p, ps.polygons) {
			// Filter away down-facing normal vectors
			if ((p[1]-p[0]).cross(p[2]-p[0])[2] <=0) continue;
			
			Outline2d outline;
			BOOST_FOREACH(const Vector3d &v, p) {
				outline.push_back(Vector2d(v[0], v[1]));
			}
			poly->addOutline(outline);
		}
		return poly;
	}

}


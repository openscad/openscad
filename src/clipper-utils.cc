#include "clipper-utils.h"
#include <boost/foreach.hpp>

namespace ClipperUtils {

	ClipperLib::Polygons fromPolygon2d(const Polygon2d &poly, bool sanitize) {
		ClipperLib::Polygons result;
		BOOST_FOREACH(const Outline2d &outline, poly.outlines()) {
			ClipperLib::Polygon p;
			BOOST_FOREACH(const Vector2d &v, outline) {
				p.push_back(ClipperLib::IntPoint(v[0]*CLIPPER_SCALE, v[1]*CLIPPER_SCALE));
			}
			result.push_back(p);
		}

		if (sanitize) {
			ClipperLib::Clipper clipper;
			clipper.AddPolygons(result, ClipperLib::ptSubject);
			clipper.Execute(ClipperLib::ctUnion, result, ClipperLib::pftEvenOdd);
		}
		return result;
	}

	Polygon2d *toPolygon2d(const ClipperLib::Polygons &poly) {
		Polygon2d *result = new Polygon2d;
		BOOST_FOREACH(const ClipperLib::Polygon &p, poly) {
			Outline2d outline;
			BOOST_FOREACH(const ClipperLib::IntPoint &ip, p) {
				outline.push_back(Vector2d(1.0*ip.X/CLIPPER_SCALE,
																	 1.0*ip.Y/CLIPPER_SCALE));
			}
			result->addOutline(outline);
		}
		return result;
	}

};

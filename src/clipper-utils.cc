#include "clipper-utils.h"
#include <boost/foreach.hpp>

namespace ClipperUtils {

	ClipperLib::Path fromOutline2d(const Outline2d &outline) {
		ClipperLib::Path p;
		BOOST_FOREACH(const Vector2d &v, outline) {
			p.push_back(ClipperLib::IntPoint(v[0]*CLIPPER_SCALE, v[1]*CLIPPER_SCALE));
		}
		// Make sure all polygons point up, since we project also 
		// back-facing polygon in PolysetUtils::project()
		if (!ClipperLib::Orientation(p)) std::reverse(p.begin(), p.end());
		
		return p;
	}

	ClipperLib::Paths fromPolygon2d(const Polygon2d &poly) {
		ClipperLib::Paths result;
		BOOST_FOREACH(const Outline2d &outline, poly.outlines()) {
			result.push_back(fromOutline2d(outline));
		}
		return result;
	}

	Polygon2d *toPolygon2d(const ClipperLib::Paths &poly) {
		Polygon2d *result = new Polygon2d;
		BOOST_FOREACH(const ClipperLib::Path &p, poly) {
			Outline2d outline;
			const Vector2d *lastv = NULL;
			BOOST_FOREACH(const ClipperLib::IntPoint &ip, p) {
				Vector2d v(1.0*ip.X/CLIPPER_SCALE, 1.0*ip.Y/CLIPPER_SCALE);
				// Ignore too close vertices. This is to be nice to subsequent processes.
				if (lastv && (v-*lastv).squaredNorm() < 0.001) continue;
				outline.push_back(v);
				lastv = &outline.back();
			}
			result->addOutline(outline);
		}
		return result;
	}

	ClipperLib::Paths process(const ClipperLib::Paths &polygons, 
															 ClipperLib::ClipType cliptype,
															 ClipperLib::PolyFillType polytype)
	{
		ClipperLib::Paths result;
		ClipperLib::Clipper clipper;
		clipper.AddPaths(polygons, ClipperLib::ptSubject, true);
		clipper.Execute(cliptype, result, polytype);
		return result;
	}

};

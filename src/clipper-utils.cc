#include "clipper-utils.h"
#include <boost/foreach.hpp>

namespace ClipperUtils {

	ClipperLib::Path fromOutline2d(const Outline2d &outline, bool keep_orientation) {
		ClipperLib::Path p;
		BOOST_FOREACH(const Vector2d &v, outline) {
			p.push_back(ClipperLib::IntPoint(v[0]*CLIPPER_SCALE, v[1]*CLIPPER_SCALE));
		}
		// Make sure all polygons point up, since we project also 
		// back-facing polygon in PolysetUtils::project()
		if (!keep_orientation && !ClipperLib::Orientation(p)) std::reverse(p.begin(), p.end());
		
		return p;
	}

	ClipperLib::Paths fromPolygon2d(const Polygon2d &poly) {
		ClipperLib::Paths result;
		BOOST_FOREACH(const Outline2d &outline, poly.outlines()) {
			result.push_back(fromOutline2d(outline, poly.isSanitized() ? true : false));
		}
		return result;
	}

	Polygon2d *sanitize(const Polygon2d &poly) {
		return toPolygon2d(sanitize(ClipperUtils::fromPolygon2d(poly)));
	}

	ClipperLib::Paths sanitize(const ClipperLib::Paths &paths) {
		return ClipperUtils::process(paths, 
																 ClipperLib::ctUnion, 
																 ClipperLib::pftEvenOdd);
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
		result->setSanitized(true);
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

	Polygon2d *apply(std::vector<const Polygon2d*> polygons, 
									 ClipperLib::ClipType clipType)
	{
		ClipperLib::Clipper clipper;
		bool first = true;
		BOOST_FOREACH(const Polygon2d *polygon, polygons) {
			ClipperLib::Paths paths = fromPolygon2d(*polygon);
			if (!polygon->isSanitized()) paths = sanitize(paths);
			clipper.AddPaths(paths, first ? ClipperLib::ptSubject : ClipperLib::ptClip, true);
			if (first) first = false;
		}
		ClipperLib::Paths sumresult;
		clipper.Execute(clipType, sumresult, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
		if (sumresult.size() == 0) return NULL;
		// The returned result will have outlines ordered according to whether 
		// they're positive or negative: Positive outlines counter-clockwise and 
		// negative outlines clockwise.
		return ClipperUtils::toPolygon2d(sumresult);
	}
};


#include "clipper-utils.h"
#include <boost/foreach.hpp>

namespace ClipperUtils {

	ClipperLib::Path fromOutline2d(const Outline2d &outline, bool keep_orientation) {
		ClipperLib::Path p;
		BOOST_FOREACH(const Vector2d &v, outline.vertices) {
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

	ClipperLib::PolyTree sanitize(const ClipperLib::Paths &paths) {
		ClipperLib::PolyTree result;
		ClipperLib::Clipper clipper;
		clipper.AddPaths(paths, ClipperLib::ptSubject, true);
		clipper.Execute(ClipperLib::ctUnion, result, ClipperLib::pftEvenOdd);
		return result;
	}

	Polygon2d *toPolygon2d(const ClipperLib::PolyTree &poly) {
		Polygon2d *result = new Polygon2d;
		const ClipperLib::PolyNode *node = poly.GetFirst();
		while (node) {
			Outline2d outline;
			outline.positive = !node->IsHole();
			const Vector2d *lastv = NULL;
			BOOST_FOREACH(const ClipperLib::IntPoint &ip, node->Contour) {
				Vector2d v(1.0*ip.X/CLIPPER_SCALE, 1.0*ip.Y/CLIPPER_SCALE);
				// Ignore too close vertices. This is to be nice to subsequent processes.
				if (lastv && (v-*lastv).squaredNorm() < 0.001) continue;
				outline.vertices.push_back(v);
				lastv = &outline.vertices.back();
			}
			result->addOutline(outline);
			node = node->GetNext();
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

	Polygon2d *apply(const std::vector<ClipperLib::Paths> &pathsvector,
									 ClipperLib::ClipType clipType)
	{
		ClipperLib::Clipper clipper;
		bool first = true;
		BOOST_FOREACH(const ClipperLib::Paths &paths, pathsvector) {
			clipper.AddPaths(paths, first ? ClipperLib::ptSubject : ClipperLib::ptClip, true);
			if (first) first = false;
		}
		ClipperLib::PolyTree sumresult;
		clipper.Execute(clipType, sumresult, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
		if (sumresult.Total() == 0) return NULL;
		// The returned result will have outlines ordered according to whether 
		// they're positive or negative: Positive outlines counter-clockwise and 
		// negative outlines clockwise.
		return ClipperUtils::toPolygon2d(sumresult);
	}

	Polygon2d *apply(const std::vector<const Polygon2d*> &polygons, 
									 ClipperLib::ClipType clipType)
	{
		std::vector<ClipperLib::Paths> pathsvector;
		BOOST_FOREACH(const Polygon2d *polygon, polygons) {
			ClipperLib::Paths polypaths = fromPolygon2d(*polygon);
			if (!polygon->isSanitized()) ClipperLib::PolyTreeToPaths(sanitize(polypaths), polypaths);
			pathsvector.push_back(polypaths);
		}
		return apply(pathsvector, clipType);
	}
};


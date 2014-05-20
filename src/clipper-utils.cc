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
	
 /*!
	 We want to use a PolyTree to convert to Polygon2d, since only PolyTrees
	 have an explicit notion of holes.
	 We could use a Paths structure, but we'd have to check the orientation of each
	 path before adding it to the Polygon2d.
 */
	Polygon2d *toPolygon2d(const ClipperLib::PolyTree &poly) {
		const double CLEANING_DISTANCE = 0.001 * CLIPPER_SCALE;

		Polygon2d *result = new Polygon2d;
		const ClipperLib::PolyNode *node = poly.GetFirst();
		while (node) {
			Outline2d outline;
			outline.positive = !node->IsHole();

			ClipperLib::Path cleaned_path;
			ClipperLib::CleanPolygon(node->Contour, cleaned_path, CLEANING_DISTANCE);

			// CleanPolygon can in some cases reduce the polygon down to no vertices
			if (cleaned_path.size() >= 3)  {
				BOOST_FOREACH(const ClipperLib::IntPoint &ip, cleaned_path) {
					Vector2d v(1.0*ip.X/CLIPPER_SCALE, 1.0*ip.Y/CLIPPER_SCALE);
					outline.vertices.push_back(v);
				}
				result->addOutline(outline);
			}

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

		if (clipType == ClipperLib::ctIntersection && pathsvector.size() > 2) {
			// intersection operations must be split into a sequence of binary operations
			ClipperLib::Paths source = pathsvector[0];
			ClipperLib::PolyTree result;
			for (unsigned int i = 1; i < pathsvector.size(); i++) {
				clipper.AddPaths(source, ClipperLib::ptSubject, true);
				clipper.AddPaths(pathsvector[i], ClipperLib::ptClip, true);
				clipper.Execute(clipType, result, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
				if (i != pathsvector.size()-1) {
                    ClipperLib::PolyTreeToPaths(result, source);
                    clipper.Clear();
                }
			}
			return ClipperUtils::toPolygon2d(result);
		}

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


	// This is a copy-paste from ClipperLib with the modification that the union operation is not performed
	// The reason is numeric robustness. With the insides missing, the intersection points created by the union operation may
	// (due to rounding) be located at slightly different locations than the original geometry and this
	// can give rise to cracks
	static void minkowski_outline(const ClipperLib::Path& poly, const ClipperLib::Path& path,
								  ClipperLib::Paths& quads, bool isSum, bool isClosed)
	{
		int delta = (isClosed ? 1 : 0);
		size_t polyCnt = poly.size();
		size_t pathCnt = path.size();
		ClipperLib::Paths pp;
		pp.reserve(pathCnt);
		if (isSum)
			for (size_t i = 0; i < pathCnt; ++i)
			{
				ClipperLib::Path p;
				p.reserve(polyCnt);
				for (size_t j = 0; j < poly.size(); ++j)
					p.push_back(ClipperLib::IntPoint(path[i].X + poly[j].X, path[i].Y + poly[j].Y));
				pp.push_back(p);
			}
		else
			for (size_t i = 0; i < pathCnt; ++i)
			{
				ClipperLib::Path p;
				p.reserve(polyCnt);
				for (size_t j = 0; j < poly.size(); ++j)
					p.push_back(ClipperLib::IntPoint(path[i].X - poly[j].X, path[i].Y - poly[j].Y));
				pp.push_back(p);
			}

		quads.reserve((pathCnt + delta) * (polyCnt + 1));
		for (size_t i = 0; i < pathCnt - 1 + delta; ++i)
			for (size_t j = 0; j < polyCnt; ++j)
			{
				ClipperLib::Path quad;
				quad.reserve(4);
				quad.push_back(pp[i % pathCnt][j % polyCnt]);
				quad.push_back(pp[(i + 1) % pathCnt][j % polyCnt]);
				quad.push_back(pp[(i + 1) % pathCnt][(j + 1) % polyCnt]);
				quad.push_back(pp[i % pathCnt][(j + 1) % polyCnt]);
				if (!Orientation(quad)) ClipperLib::ReversePath(quad);
				quads.push_back(quad);
			}
	}
	
	// Add the polygon a translated to an arbitrary point of each separate component of b.
  // Ideally, we would translate to the midpoint of component b, but the point can
	// be chosen arbitrarily since the translated object would always stay inside
	// the minkowski sum. 
	static void fill_minkowski_insides(const ClipperLib::Paths &a,
																		 const ClipperLib::Paths &b,
																		 ClipperLib::Paths &target) {
		BOOST_FOREACH(const ClipperLib::Path &b_path, b) {
			// We only need to add for positive components of b
			if (!b_path.empty() && ClipperLib::Orientation(b_path) == 1) {
				const ClipperLib::IntPoint &delta = b_path[0]; // arbitrary point
				BOOST_FOREACH(const ClipperLib::Path &path, a) {
					target.push_back(path);
					BOOST_FOREACH(ClipperLib::IntPoint &point, target.back()) {
						point.X += delta.X;
						point.Y += delta.Y;
					}
				}
			}
		}
	}

	Polygon2d *applyMinkowski(const std::vector<const Polygon2d*> &polygons)
	{
		if (polygons.size() == 1) return new Polygon2d(*polygons[0]); // Just copy

		ClipperLib::Clipper c;
		ClipperLib::Paths lhs = ClipperUtils::fromPolygon2d(*polygons[0]);

		for (size_t i=1; i<polygons.size(); i++) {
			ClipperLib::Paths minkowski_terms;
			ClipperLib::Paths rhs = ClipperUtils::fromPolygon2d(*polygons[i]);

			// First, convolve each outline of lhs with the outlines of rhs
			BOOST_FOREACH(ClipperLib::Path const& rhs_path, rhs) {
				BOOST_FOREACH(ClipperLib::Path const& lhs_path, lhs) {
					ClipperLib::Paths result;
					minkowski_outline(lhs_path, rhs_path, result, true, true);
					minkowski_terms.insert(minkowski_terms.end(), result.begin(), result.end());
				}
			}
			
			// Then, fill the central parts
			fill_minkowski_insides(lhs, rhs, minkowski_terms);
			fill_minkowski_insides(rhs, lhs, minkowski_terms);

			// This union operation must be performed at each interation since the minkowski_terms
			// now contain lots of small quads
			c.Clear();
			c.AddPaths(minkowski_terms, ClipperLib::ptSubject, true);

			if (i != polygons.size() - 1)
				c.Execute(ClipperLib::ctUnion, lhs, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
		}

		ClipperLib::PolyTree polytree;
		c.Execute(ClipperLib::ctUnion, polytree, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

		return toPolygon2d(polytree);
	}

	Polygon2d *applyOffset(const Polygon2d& poly, double offset, ClipperLib::JoinType joinType, double miter_limit, double arc_tolerance) {
		ClipperLib::ClipperOffset co(miter_limit, arc_tolerance * CLIPPER_SCALE);
		co.AddPaths(fromPolygon2d(poly), joinType, ClipperLib::etClosedPolygon);
		ClipperLib::PolyTree result;
		co.Execute(result, offset * CLIPPER_SCALE);
		return toPolygon2d(result);
	}
};

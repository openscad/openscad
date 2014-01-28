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

		if (clipType == ClipperLib::ctIntersection && pathsvector.size() > 2) {
			// intersection operations must be split into a sequence of binary operations
			ClipperLib::Paths source = pathsvector[0];
			ClipperLib::PolyTree result;
			for (int i = 1; i < pathsvector.size(); i++) {
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
		ClipperLib::Paths lhs = ClipperUtils::fromPolygon2d(*polygons[0]);
		for (size_t i=1; i<polygons.size(); i++) {
			ClipperLib::Paths minkowski_terms;
			ClipperLib::Paths rhs = ClipperUtils::fromPolygon2d(*polygons[i]);
			// First, convolve each outline of lhs with the outlines of rhs
			BOOST_FOREACH(ClipperLib::Path const& rhs_path, rhs) {
				BOOST_FOREACH(ClipperLib::Path const& lhs_path, lhs) {
					ClipperLib::Paths result;
					ClipperLib::MinkowskiSum(lhs_path, rhs_path, result, true);
					minkowski_terms.insert(minkowski_terms.end(), result.begin(), result.end());
				}
			}
			
			// Then, fill the central parts
			fill_minkowski_insides(lhs, rhs, minkowski_terms);
			fill_minkowski_insides(rhs, lhs, minkowski_terms);
			lhs = minkowski_terms;
		}
		
		// Finally, merge the Minkowski terms
		std::vector<ClipperLib::Paths> pathsvec;
		pathsvec.push_back(lhs);
		return ClipperUtils::apply(pathsvec, ClipperLib::ctUnion);
	}

};

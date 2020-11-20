#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>

#include <boost/shared_ptr.hpp>
#include <cmath>
#include <algorithm>

#include "GeometryUtils.h"
#include "clipper-utils.h"
#include "ssroof.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel         CGAL_KERNEL;
typedef CGAL_KERNEL::Point_2                                        CGAL_Point_2;
typedef CGAL::Polygon_2<CGAL_KERNEL>                                CGAL_Polygon_2;
typedef CGAL::Polygon_with_holes_2<CGAL_KERNEL>                     CGAL_Polygon_with_holes_2;
typedef CGAL::Straight_skeleton_2<CGAL_KERNEL>                      CGAL_Ss;
typedef boost::shared_ptr<CGAL_Ss>                                  CGAL_SsPtr;

typedef ClipperLib::PolyTree                                        PolyTree;
typedef ClipperLib::PolyNode                                        PolyNode;

CGAL_Polygon_2 to_cgal_polygon_2(const VectorOfVector2d &points) 
{
	CGAL_Polygon_2 poly;
	for (auto v : points)
		poly.push_back({v[0], v[1]});
	return poly;
}

// break a list of outlines into polygons with holes
std::vector<CGAL_Polygon_with_holes_2> polygons_with_holes(const Polygon2d &poly) 
{
	std::vector<CGAL_Polygon_with_holes_2> ret;
	PolyTree polytree = ClipperUtils::sanitize(ClipperUtils::fromPolygon2d(poly));  // how do we check if this was successful?

	// lambda for recursive walk through polytree
	std::function<void (PolyNode *)> walk = [&](PolyNode *c) {
		// outer path
		CGAL_Polygon_with_holes_2 c_poly(to_cgal_polygon_2(ClipperUtils::fromPath(c->Contour)));
		// holes
		for (auto cc : c->Childs) {
			c_poly.add_hole(to_cgal_polygon_2(ClipperUtils::fromPath(cc->Contour)));
			for (auto ccc : cc->Childs)
				walk(ccc);
		}
		ret.push_back(c_poly);
		return;
	};

	for (auto root_node : polytree.Childs)
		walk(root_node);

	return ret;
}

PolySet *straight_skeleton_roof(const Polygon2d &poly)
{
	PolySet *hat = new PolySet(3);

	for (auto shape : polygons_with_holes(poly)) {
		CGAL_SsPtr ss = CGAL::create_interior_straight_skeleton_2(shape);
		for (auto i=ss->faces_begin(); i!=ss->faces_end(); ++i ) {
			Polygon roof, floor;
			for (auto h = i->halfedge() ; ; ) {
				roof.push_back({h->vertex()->point().x(), h->vertex()->point().y(), h->vertex()->time()});
				floor.push_back({h->vertex()->point().x(), h->vertex()->point().y(), 0});
				h = h->next();
				if (h == i->halfedge())
					break;
			}

			hat->append_poly(roof);
			std::reverse(floor.begin(), floor.end());  // floor has wrong orientation
			hat->append_poly(floor);
		}
	}

	return hat;
}

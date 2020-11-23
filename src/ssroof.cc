#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>
#include <CGAL/partition_2.h>
#include <CGAL/Partition_traits_2.h>

#include <boost/shared_ptr.hpp>
#include <cmath>
#include <algorithm>
#include <map>

#include "GeometryUtils.h"
#include "clipper-utils.h"
#include "ssroof.h"

#include <vector>
#include <cassert>
#include <list>

// debug
#include <iomanip>

typedef CGAL::Exact_predicates_inexact_constructions_kernel         CGAL_KERNEL;
typedef CGAL_KERNEL::Point_2                                        CGAL_Point_2;
typedef CGAL::Polygon_2<CGAL_KERNEL>                                CGAL_Polygon_2;
typedef CGAL::Segment_2<CGAL_KERNEL>                                CGAL_Segment_2;
typedef CGAL::Polygon_with_holes_2<CGAL_KERNEL>                     CGAL_Polygon_with_holes_2;
typedef CGAL::Straight_skeleton_2<CGAL_KERNEL>                      CGAL_Ss;

typedef CGAL::Partition_traits_2<CGAL_KERNEL>                       CGAL_PT;

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
std::list<CGAL_Polygon_with_holes_2> polygons_with_holes(const Polygon2d &poly) 
{
	std::list<CGAL_Polygon_with_holes_2> ret;
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

std::vector<CGAL_Polygon_2> faces_cleanup(CGAL_Ss::Face_iterator faces_begin, CGAL_Ss::Face_iterator faces_end)
{
	std::vector<CGAL_Polygon_2> faces;
	
	// convert faces to polygonss
	for (auto face = faces_begin; face!=faces_end; face++) {
		CGAL_Polygon_2 dirty_face;

		for (auto h=face->halfedge(); ;) {
			dirty_face.push_back(h->vertex()->point());
			h = h->next();
			if (h == face->halfedge()) {
				break;
			}
		}
		faces.push_back(dirty_face);
	}

	// look for an antenna
	bool there_may_be_antenna = true;
	while (there_may_be_antenna) {
		bool antenna_found = false;
		for (size_t f=0; f<faces.size() && !antenna_found; f++) {
			CGAL_Point_2 p_prev = faces[f][faces[f].size()-1];
			for (size_t k=0; k < faces[f].size() && !antenna_found; k++) {
				CGAL_Point_2 p = faces[f][k];
				CGAL_Point_2 p_next = (k==faces[f].size()-1) ? faces[f][0] : faces[f][k+1];
				
				auto d1 = CGAL::squared_distance(p_next, CGAL_Segment_2(p_prev, p)),
					 d2 = CGAL::squared_distance(p_prev, CGAL_Segment_2(p, p_next));
				if (d1 < 1.0e-20) {
					antenna_found = true;
					std::cout << "Antenna of type 1 found in face " << f << ", vertex " << k 
						<< " out of " << faces[f].size()
						<< "\n";
					faces[f].erase(faces[f].vertices_begin() + k);
					// find a face with halfedge [p,p_prev] using brute force
					// and replace it with edges [p, p_next] and [p_next, p_prev]
					for (size_t ff=0; ff<faces.size(); ff++) {
						for (size_t kk=0; kk<faces[ff].size(); kk++) {
							size_t kk_next = (kk==faces[ff].size()-1) ? 0 : (kk + 1);
							if (faces[ff][kk]==p && faces[ff][kk_next]==p_prev) {
								std::cout << "found matching long edge in face " << ff << " vertex " << kk << "\n";
								faces[ff].insert(faces[ff].vertices_begin() + kk_next, p_next);
								std::cout << "and attempted a fix\n";
							}
						}
					}
				} else if (d2 < 1.0e-20) {
					antenna_found = true;
					std::cout << "Antenna of type 2 found in face " << f << ", vertex " << k 
						<< " out of " << faces[f].size()
						<< "\n";
					faces[f].erase(faces[f].vertices_begin() + k);
					// find a face with halfedge [p_next,p] using brute force
					// and replace it with edges [p_next, p_prev] and [p_prev, p]
					for (size_t ff=0; ff<faces.size(); ff++) {
						for (size_t kk=0; kk<faces[ff].size(); kk++) {
							size_t kk_next = (kk==faces[ff].size()-1) ? 0 : (kk + 1);
							if (faces[ff][kk]==p_next && faces[ff][kk_next]==p) {
								std::cout << "found matching long edge in face " << ff << " vertex " << kk << "\n";
								faces[ff].insert(faces[ff].vertices_begin() + kk_next, p_prev);
								std::cout << "and attempted a fix\n";
							}
						}
					}
				}
				p_prev = p;
			}
		}

		if (!antenna_found) {
			there_may_be_antenna = false;
			std::cout << "no more antennas this time\n";
		}
	}

	return faces;
}

void print_polygon(const CGAL_Polygon_2 &poly) {
	for (size_t k=0; k<poly.size(); k++) {
		size_t j = (k==poly.size()-1) ? 0 : (k + 1);
		std::cout << "set arrow from " << poly[k].x() << "," << poly[k].y()
			<< " to " << poly[j].x() << "," << poly[j].y()
			<< "\n";
	}
}

PolySet *straight_skeleton_roof(const Polygon2d &poly)
{
	PolySet *hat = new PolySet(3);

	std::list<CGAL_Polygon_with_holes_2> shapes = polygons_with_holes(poly);
	
	for (CGAL_Polygon_with_holes_2 shape : shapes) {
		CGAL_SsPtr ss = CGAL::create_interior_straight_skeleton_2(shape);
		// store heights of vertices
		std::map<std::vector<double>, double> heights; 
		for (auto v=ss->vertices_begin(); v!=ss->vertices_end(); v++) {
			std::vector<double> p = {v->point().x(), v->point().y()};
			heights[p] = v->time();
		}

		for (auto face : faces_cleanup(ss->faces_begin(), ss->faces_end())) {
			std::list<CGAL_PT::Polygon_2> facets;
			if (!face.is_convex()) {
				if (!face.is_simple()) {
					std::cout << "huj huj\n"
						<< face << "\n";
					print_polygon(face);
				}
				CGAL::approx_convex_partition_2(face.vertices_begin(), face.vertices_end(),
						std::back_inserter(facets));
			} else {
				CGAL_PT::Polygon_2 face_pt;
				for (auto v=face.vertices_begin(); v!=face.vertices_end(); v++) {
					face_pt.push_back(*v);
				}
				facets.push_back(face_pt);
			}

			for (auto facet : facets) {
				Polygon floor, roof;
				for (auto v=facet.vertices_begin(); v!=facet.vertices_end(); v++) {
					floor.push_back({v->x(), v->y(), 0.0});
					roof.push_back({v->x(), v->y(), heights[{v->x(), v->y()}]});
				}
				hat->append_poly(roof);
				std::reverse(floor.begin(), floor.end());  // floor has wrong orientation
				hat->append_poly(floor);
			}
		}


		//std::cout << "HUJ\n"
		//	<< face
			//	<< "\n simple: " << face.is_simple()
			//	<< "\n shape: " << shape
			//	<< "\n";
		//	if (!face.is_simple()) {
		//		std::vector<double> x,y;
		//		for (auto v=face.vertices_begin(); v!=face.vertices_end(); v++) {
		//			x.push_back(v->x());
		//			y.push_back(v->y());
		//		}
		//		std::cout << "JIGURGA: "
		//			<< (x[1]-x[0])*(y[2]-y[0]) - (x[2]-x[0])*(y[1]-y[0])
		//			<< "\n";
		//		return hat;
		//	}

		//	std::list<CGAL_PT::Polygon_2> facets;
		//	CGAL::optimal_convex_partition_2(face.vertices_begin(), face.vertices_end(),
		//			std::back_inserter(facets));
		//	//std::cout << "PIZDA\n";
		//	for (auto facet : facets) {
		//		Polygon floor, roof;
		//		for (auto v=facet.vertices_begin(); v!=facet.vertices_end(); v++) {
		//			floor.push_back({v->x(), v->y(), 0.0});
		//			roof.push_back({v->x(), v->y(), heights[{v->x(), v->y()}]});
		//		}
		//		hat->append_poly(roof);
		//		std::reverse(floor.begin(), floor.end());  // floor has wrong orientation
		//		hat->append_poly(floor);
		//	}
	}

	return hat;
}

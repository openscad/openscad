#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>
#include <CGAL/partition_2.h>
#include <CGAL/Partition_traits_2.h>

#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Segment_Delaunay_graph_2.h>
#include <CGAL/Segment_Delaunay_graph_filtered_traits_2.h>
#include <CGAL/Segment_Delaunay_graph_adaptation_traits_2.h>
#include <CGAL/Segment_Delaunay_graph_adaptation_policies_2.h>

#include <boost/shared_ptr.hpp>
#include <cmath>
#include <algorithm>
#include <map>

#include "GeometryUtils.h"
#include "clipper-utils.h"
#include "ssroof.h"

#include <vector>
#include <cassert>

// debug
#include <iomanip>

typedef CGAL::Exact_predicates_inexact_constructions_kernel         CGAL_KERNEL;
typedef CGAL_KERNEL::Point_2                                        CGAL_Point_2;
typedef CGAL::Polygon_2<CGAL_KERNEL>                                CGAL_Polygon_2;
typedef CGAL::Vector_2<CGAL_KERNEL>                                 CGAL_Vector_2;
typedef CGAL::Line_2<CGAL_KERNEL>                                   CGAL_Line_2;
typedef CGAL::Segment_2<CGAL_KERNEL>                                CGAL_Segment_2;
typedef CGAL::Polygon_with_holes_2<CGAL_KERNEL>                     CGAL_Polygon_with_holes_2;
typedef CGAL::Straight_skeleton_2<CGAL_KERNEL>                      CGAL_Ss;

typedef CGAL::Segment_Delaunay_graph_filtered_traits_without_intersections_2<CGAL_KERNEL>     CGAL_SDG2_traits;
typedef CGAL::Segment_Delaunay_graph_2<CGAL_SDG2_traits>                                      CGAL_SDG2;
typedef CGAL::Segment_Delaunay_graph_adaptation_traits_2<CGAL_SDG2>                           CGAL_SDG2_AT;
typedef CGAL::Segment_Delaunay_graph_caching_degeneracy_removal_policy_2<CGAL_SDG2>           CGAL_SDG2_AP;
typedef CGAL::Voronoi_diagram_2<CGAL_SDG2,CGAL_SDG2_AT,CGAL_SDG2_AP>                          CGAL_VD;

typedef CGAL::Partition_traits_2<CGAL_KERNEL>                       CGAL_PT;

typedef boost::shared_ptr<CGAL_Ss>                                  CGAL_SsPtr;

typedef ClipperLib::PolyTree                                        PolyTree;
typedef ClipperLib::PolyNode                                        PolyNode;

struct Faces_2_plus_1 {
	std::vector<std::vector<CGAL_Point_2>> faces;
	std::map<CGAL_Point_2,double>          heights;
};

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

std::vector<CGAL_Polygon_2> faces_cleanup(CGAL_SsPtr ss)
{
	std::vector<CGAL_Polygon_2> faces;

	std::map<CGAL_Point_2, CGAL_Point_2> skipped_vertices;
	std::function<CGAL_Point_2 (const CGAL_Point_2 &)> skipped_vertex = [&](const CGAL_Point_2 &p) {
		auto pp = skipped_vertices.find(p);
		if (pp == skipped_vertices.end()) {
			return p;
		} else {
			return skipped_vertex(pp->second);
		}
	};

	for (auto h=ss->halfedges_begin(); h!=ss->halfedges_end(); h++) {
		auto v0 = h->opposite()->vertex(),
			 v1 = h->vertex();
		auto p0 = v0->point(),
			 p1 = v1->point();
		auto t0 = v0->time(),
			 t1 = v1->time();
		auto d2 = CGAL::squared_distance(p0, p1);
		const double dt = 1./1024.;
		if (t0 != 0 && t0 < t1 && std::floor(t0 / dt) == std::floor(t1 / dt) && d2 < dt*dt*64) {
			skipped_vertices[p0] = p1;
			std::cout << "Skipping vertex #" << skipped_vertices.size() - 1 << "\n";
		}
	}

	// convert faces to polygons
	for (auto face = ss->faces_begin(); face!=ss->faces_end(); face++) {
		CGAL_Polygon_2 dirty_face;

		for (auto h=face->halfedge(); ;) {
			CGAL_Point_2 pp = h->vertex()->point();
				dirty_face.push_back(pp);
			h = h->next();
			if (h == face->halfedge()) {
				break;
			}
		}
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

	// now there are no antennas, probably
	// do convex partition
	std::vector<CGAL_PT::Polygon_2> convex_faces;
	for (auto face : faces) {
		if (!face.is_convex()) {
			if (!face.is_simple()) {
				std::cout << "huj huj\n"
					<< face << "\n";
			}
			CGAL::approx_convex_partition_2(face.vertices_begin(), face.vertices_end(),
					std::back_inserter(convex_faces));
		}
	}

	// now remove skipped points
	faces.clear();
	for (auto convex_face : convex_faces) {
		for (auto p=convex_face.vertices_begin(); p!=convex_face.vertices_end(); p++) {

		}
	}
	
/*
	// convert faces to polygons
	for (auto face = ss->faces_begin(); face!=ss->faces_end(); face++) {
		CGAL_Polygon_2 dirty_face;

		for (auto h=face->halfedge(); ;) {
			CGAL_Point_2 pp = skipped_vertex(h->vertex()->point());
			if (dirty_face.size() == 0 || dirty_face[dirty_face.size()-1] != pp) {
				dirty_face.push_back(pp);
			}
			h = h->next();
			if (h == face->halfedge()) {
				break;
			}
		}
		if (dirty_face.size() >= 2 && dirty_face[0] == dirty_face[dirty_face.size() - 1]) {
			dirty_face.erase(dirty_face.begin() + dirty_face.size() - 1);
		}
		if (dirty_face.size() >= 3) {
			faces.push_back(dirty_face);
		} else {
			std::cout << "Skipped face with " << dirty_face.size() << " vertices\n";
		}
	}
*/

	return faces;
}

PolySet *straight_skeleton_roof(const Polygon2d &poly)
{
	PolySet *hat = new PolySet(3);

	std::vector<CGAL_Polygon_with_holes_2> shapes = polygons_with_holes(poly);
	
	for (CGAL_Polygon_with_holes_2 shape : shapes) {
		CGAL_SsPtr ss = CGAL::create_interior_straight_skeleton_2(shape);
		// store heights of vertices
		std::map<std::vector<double>, double> heights; 
		for (auto v=ss->vertices_begin(); v!=ss->vertices_end(); v++) {
			std::vector<double> p = {v->point().x(), v->point().y()};
			heights[p] = v->time();
		}

		for (auto face : faces_cleanup(ss)) {
			std::vector<CGAL_PT::Polygon_2> facets;
			if (!face.is_convex()) {
				if (!face.is_simple()) {
					std::cout << "huj huj\n"
						<< face << "\n";
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

		//	std::vector<CGAL_PT::Polygon_2> facets;
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

/*

std::vector<CGAL_Point_2> discretize_arc(const CGAL_Point_2 &point, const CGAL_Segment_2 &segment,
		const CGAL_Point_2 &source, const CGAL_Point_2 &target)
{
	std::vector<CGAL_Point_2> ret;
	
	const double max_angle_deviation = M_PI / 64;

	const CGAL_Point_2 projected_point = CGAL_Line_2(segment).projection(point);
	const double point_distance = std::sqrt(CGAL::squared_distance(point, projected_point));
	assert(point_distance > 0);
	const CGAL_Vector_2 point_direction = (point - projected_point) / point_distance;

	// an orthogonal affine transformation which maps point to zero and
	// segment parallel to the x axes on the negative side
	//     a_point ->  A(a_point - point)
	const double ai_11 =  point_direction.y(),
		  ai_12 =  point_direction.x(),
		  ai_21 = -point_direction.x(),
		  ai_22 =  point_direction.y(),
		  a_11  =  ai_22,
		  a_12  = -ai_12;
	//a_21  = -ai_21, a_22  =  ai_22;

	// x coordinates of source and target
	const double transformed_source_x = a_11 * (source.x() - point.x()) + a_12 * (source.y() - point.y());
	const double transformed_target_x = a_11 * (target.x() - point.x()) + a_12 * (target.y() - point.y());
	assert(transformed_source_x < transformed_target_x);
	
	// in transformed coordinates the parabola has equation y = (x^2 - point_distance^2) / (2 point_distance)
	auto y = [point_distance](double x) { return (x*x - point_distance*point_distance) / (2 * point_distance); };
	auto y_prime = [point_distance](double x) { return x / point_distance; };
	// angle between a segment and the parabola
	auto segment_angle = [y,y_prime](double x1, double x2){
		double dx = x2 - x1,
			   dy = y(x2) - y(x1);
		double tx = 1,
			   ty = (std::abs(x1) < std::abs(x2)) ? y_prime(x1) : y_prime(x2);
		return std::abs(std::atan2(dx*ty - dy*tx, dx*tx + dy*ty));
	};

	std::vector<double> transformed_points_x = {transformed_source_x, transformed_target_x};

	for (;;) {
		if (segment_angle(transformed_points_x.end()[-2], transformed_points_x.end()[-1]) > max_angle_deviation) {
			transformed_points_x.end()[-1] = 0.5 * transformed_points_x.end()[-2] + 0.5 * transformed_points_x.end()[-1];
		} else {
			if (transformed_points_x.end()[-1] == transformed_target_x) {
				break;
			} else {
				transformed_points_x.push_back(transformed_target_x);
			}
		}
	}
	
	for (auto x : transformed_points_x) {
		if (x == transformed_source_x) {
			ret.push_back(source);
		} else if (x == transformed_target_x) {
			ret.push_back(target);
		} else {
			ret.push_back(CGAL_Point_2(point.x() + ai_11 * x + ai_12 * y(x), point.y() + ai_21 * x + ai_22 * y(x)));
		}
	}

	return ret;
}

Faces_2_plus_1 vd_inner_faces(const CGAL_VD &vd) {
	Faces_2_plus_1 ret;

	for (auto ff = vd.faces_begin(); ff != vd.faces_end(); ff++) {
		if (ff->dual()->is_segment()) {
			const CGAL_Segment_2 ff_segment = ff->dual()->site().segment();
			const CGAL_Line_2 ff_line(ff_segment);
			auto h = ff->ccb();
			// find boundary halfedge starting from segment or similar
			while (!( h->has_target() && ff_line.has_on_positive_side(h->target()->point())
						&& (!h->has_source() || !ff_line.has_on_positive_side(h->source()->point()))
					)) {
				h++;
			}
			CGAL_Polygon_2 contour_2;
			contour_2.push_back(ff_segment.target());
			while (h->has_target() && ff_line.has_on_positive_side(h->target()->point())) {
				if (h->down()->is_point() && h->down()->site().point() != ff_segment.source()
						&& h->down()->site().point() != ff_segment.target()) {
					assert(h->has_target() && h->has_source());
					std::vector<CGAL_Point_2> h_points = discretize_arc(
							h->down()->site().point(),
							ff_segment,
							h->target()->point(),
							h->source()->point());
					std::reverse(h_points.begin(), h_points.end());
					for (size_t hpn = 1; hpn < h_points.size(); hpn++) {
						contour_2.push_back(h_points[hpn]);
					}
				} else {
					contour_2.push_back(h->target()->point());
				}
				h++;
			}
			contour_2.push_back(ff_segment.source());

			// convex partition
			if (1) {
				std::vector<CGAL_PT::Polygon_2> facets;
				CGAL::approx_convex_partition_2(contour_2.vertices_begin(), contour_2.vertices_end(),
						std::back_inserter(facets));
				for (auto facet : facets) {
					ret.faces.emplace_back();
					for (auto p=facet.vertices_begin(); p!=facet.vertices_end(); p++) {
						ret.faces.back().push_back(*p);
						ret.heights[*p] = std::sqrt(CGAL::squared_distance(ff_line, *p));
					}
				}
			} else {
				ret.faces.emplace_back();
				for (auto p=contour_2.vertices_begin(); p!=contour_2.vertices_end(); p++) {
					ret.faces.back().push_back(*p);
					ret.heights[*p] = std::sqrt(CGAL::squared_distance(ff_line, *p));
				}
			}

		} else { // ff->dual()->is_point()
			const CGAL_Point_2 ff_point = ff->dual()->site().point();
			
			auto faces_add_triangle = [&ret, ff_point](CGAL_Point_2 p, CGAL_Point_2 q) {
				ret.faces.push_back({ff_point, p, q});
				ret.heights[p] = std::sqrt(CGAL::squared_distance(ff_point, p));
				ret.heights[q] = std::sqrt(CGAL::squared_distance(ff_point, q));
			};
	
			auto h = ff->ccb();
			while (!h->has_source() || h->source()->point() != ff_point) {
				h++;
			}
			// check that ff is not outside
			assert(h->down()->is_segment());
			if (ff->is_unbounded() ||
					!CGAL_Line_2(h->down()->site().segment()).has_on_positive_side(h->target()->point())) {
				continue;
			}
			assert(h->has_target() && h->has_source());
			assert(h->source()->point() != h->target()->point());
			h++;
			while (h->target()->point() != ff_point) {
				if (h->down()->is_segment()
						&& h->down()->site().segment().source() != ff_point
						&& h->down()->site().segment().target() != ff_point) {
					std::vector<CGAL_Point_2> h_points = discretize_arc(
							ff_point,
							h->down()->site().segment(),
							h->source()->point(),
							h->target()->point());
					assert(h_points.size() > 1);
					for (size_t k=1; k<h_points.size(); k++) {
						faces_add_triangle(h_points[k-1], h_points[k]);
					}
				} else {
					faces_add_triangle(h->source()->point(), h->target()->point());
				}
				h++;
			}
		}
	}
	return ret;
}

PolySet *voronoi_diagram_roof(const Polygon2d &poly)
{
	PolySet *hat = new PolySet(3);

	Polygon2d *poly_sanitized = ClipperUtils::sanitize(poly);

	std::vector<CGAL_SDG2::Site_2> poly_sites;
	for (auto outline : poly_sanitized->outlines()) {
		Vector2d prev = outline.vertices.back();
		for (Vector2d p : outline.vertices) {
			CGAL_SDG2::Site_2 site;
			site = site.construct_site_2(CGAL_Point_2(prev[0], prev[1]), CGAL_Point_2(p[0], p[1]));
			poly_sites.push_back(site);
			prev = p;
		}
	}

	delete poly_sanitized;

	std::cout << "poly sanitized\n";

	CGAL_VD vd(poly_sites.begin(), poly_sites.end());
	assert( vd.is_valid() );

	std::cout << "Voronoi computed\n" << std::flush;

	Faces_2_plus_1 inner_faces = vd_inner_faces(vd);
	
	std::cout << "Inner faces computed\n"; size_t nnn = 0;
	
	for (std::vector<CGAL_Point_2> face : inner_faces.faces) {
		std::cout << "face " << (nnn++) << "\n";
		assert(face.size() >= 3);
		Polygon floor, roof;
		for (CGAL_Point_2 v : face) {
			floor.push_back({v.x(), v.y(), 0.0});
			assert(inner_faces.heights.find(v) != inner_faces.heights.end());
			roof.push_back({v.x(), v.y(), inner_faces.heights[v]});
		}
		std::reverse(floor.begin(), floor.end());  // floor has wrong orientation
		hat->append_poly(floor);
		hat->append_poly(roof);
	}
	
	std::cout << "huj-dsd235\n";

	return hat;
}

*/

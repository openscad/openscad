#include <cmath>
#include <algorithm>
#include <map>

#include <boost/polygon/voronoi.hpp>

#include "GeometryUtils.h"
#include "clipper-utils.h"
#include "vdroof.h"

#include <vector>
#include <cassert>

#include <iostream>

typedef ClipperLib::cInt       VD_int;

typedef boost::polygon::voronoi_diagram<double>      voronoi_diagram;

struct Point {
	VD_int a;
	VD_int b;
	Point(VD_int x, VD_int y) : a(x), b(y) {}
	friend std::ostream& operator<<(std::ostream& os, const Point& point);
};

bool operator==(const Point &lhs, const Point & rhs)
{
    return lhs.a == rhs.a  &&  lhs.b == rhs.b;
}

struct Segment {
	Point p0;
	Point p1;
	Segment(VD_int x1, VD_int y1, VD_int x2, VD_int y2) : p0(x1, y1), p1(x2, y2) {}
	friend std::ostream& operator<<(std::ostream& os, const Segment& segment);
};

bool operator==(const Segment &lhs, const Segment & rhs)
{
    return lhs.p0 == rhs.p0  &&  lhs.p1 == rhs.p1;
}

std::ostream& operator<<(std::ostream& os, const Point &p)
{
    os << "(" << double(p.a) / ClipperUtils::CLIPPER_SCALE
		<< ", " << double(p.b) / ClipperUtils::CLIPPER_SCALE << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Segment &s)
{
    os << s.p0 << " ---> " << s.p1;
    return os;
}


bool segment_has_endpoint(const Segment &segment, const Point &point) {
	return segment.p0 == point || segment.p1 == point;
}


namespace boost {
	namespace polygon {

		template <>
			struct geometry_concept<Point> {
				typedef point_concept type;
			};

		template <>
			struct point_traits<Point> {
				typedef VD_int coordinate_type;

				static inline coordinate_type get(
						const Point& point, orientation_2d orient) {
					return (orient == HORIZONTAL) ? point.a : point.b;
				}
			};

		template <>
			struct geometry_concept<Segment> {
				typedef segment_concept type;
			};

		template <>
			struct segment_traits<Segment> {
				typedef VD_int coordinate_type;
				typedef Point point_type;

				static inline point_type get(const Segment& segment, direction_1d dir) {
					return dir.to_int() ? segment.p1 : segment.p0;
				}
			};
	}  // polygon
}  // boost

double distance_to_segment(const Vector2d &vertex, const Segment &segment) {
	Vector2d segment_normal(-(segment.p1.b - segment.p0.b), segment.p1.a - segment.p0.a);
	segment_normal.normalize();
	Vector2d p0_to_vertex(vertex[0] - segment.p0.a, vertex[1] - segment.p0.b);
	return std::abs(segment_normal.dot(p0_to_vertex));
}

double distance_to_point(const Vector2d &vertex, const Point &point) {
	Vector2d point_to_vertex(vertex[0] - point.a, vertex[1] - point.b);
	return point_to_vertex.norm();
}

std::vector<Vector2d> discretize_arc(const Point &point, const Segment &segment,
		const Vector2d &v0, const Vector2d &v1)
{
	std::vector<Vector2d> ret;
	
	const double max_angle_deviation = M_PI / 64;

	const Vector2d p(point.a, point.b);
	const Vector2d p0(segment.p0.a, segment.p0.b);
	const Vector2d p1(segment.p1.a, segment.p1.b);
	const Vector2d p0_to_p1_norm = (p1-p0).normalized();

	const Vector2d projected_point = p0 + p0_to_p1_norm * p0_to_p1_norm.dot(p - p0);
	
	const double point_distance = (p - projected_point).norm();
	
	assert(point_distance > 0);

	const Vector2d point_direction = (p - projected_point) / point_distance;

	// an orthogonal affine transformation which maps point to zero and
	// segment parallel to the x axes on the negative side
	//     a_point ->  A(a_point - point)
	Eigen::Matrix2d A, Ai;
	Ai << point_direction.y(), point_direction.x(), -point_direction.x(), point_direction.y();
	A = Ai.inverse();

	// x coordinates of source and target
	const double transformed_v0_x = (A * (v0 - p))[0];
	const double transformed_v1_x = (A * (v1 - p))[0];
	if (!(transformed_v0_x < transformed_v1_x)) {
		std::cout << "pizda " << transformed_v0_x << ", " << transformed_v1_x << "\n";
	}
	assert(transformed_v0_x < transformed_v1_x);
	
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

	std::vector<double> transformed_points_x = {transformed_v0_x, transformed_v1_x};

	for (;;) {
		if (segment_angle(transformed_points_x.end()[-2], transformed_points_x.end()[-1]) > max_angle_deviation) {
			transformed_points_x.end()[-1] = 0.5 * transformed_points_x.end()[-2] + 0.5 * transformed_points_x.end()[-1];
		} else {
			if (transformed_points_x.end()[-1] == transformed_v1_x) {
				break;
			} else {
				transformed_points_x.push_back(transformed_v1_x);
			}
		}
	}

	auto d =  ClipperUtils::CLIPPER_SCALE;
	std::cout << "\n\nhuj\n";
	std::cout << "segment: " << segment.p0.a / d << ", " << segment.p0.b /d
		<< " --- " << segment.p1.a/d << ", " << segment.p1.b /d
		<< "\n"
		<< "point: " << point.a/d << ", " << point.b/d
		<< "\n"
		<< "v0: " << v0[0] /d << ", " << v0[1] /d << "\n"
		<< "v1: " << v1[0] /d << ", " << v1[1] /d
		<< "\n";

	for (auto x : transformed_points_x) {
		if (x == transformed_v0_x) {
			ret.push_back(v0);
		} else if (x == transformed_v1_x) {
			ret.push_back(v1);
		} else {
			ret.push_back(p + Ai*Vector2d(x, y(x)));
		}
		std::cout << ret.back()[0] / ClipperUtils::CLIPPER_SCALE << ", " << ret.back()[1]  / ClipperUtils::CLIPPER_SCALE
			<< "\n";
	}

	return ret;
}

void print_edge(const voronoi_diagram::edge_type *edge) {
	if (!edge->vertex0())
		std::cout << "inf inf";
	else
		std::cout << edge->vertex0()->x() / ClipperUtils::CLIPPER_SCALE
			<< " " << edge->vertex0()->y() / ClipperUtils::CLIPPER_SCALE;
	std::cout << " --- ";
	
	if (!edge->vertex1())
		std::cout << "inf inf";
	else
		std::cout << edge->vertex1()->x() / ClipperUtils::CLIPPER_SCALE
			<< " " << edge->vertex1()->y() / ClipperUtils::CLIPPER_SCALE;

	if (edge->is_primary())
		std::cout << " (primary)";
	else
		std::cout << " (secondary)";

}


struct Faces_2_plus_1 {
	struct Vector2d_comp {
		bool operator()(const Vector2d &lhs, const Vector2d & rhs) const {
			return (lhs[0] < rhs[0]) || (lhs[0] == rhs[0] && lhs[1] < rhs[1]);
		}
	};
	std::vector<std::vector<Vector2d>> faces;
	std::map<Vector2d,double, Vector2d_comp> heights;
};



Faces_2_plus_1 vd_inner_faces(const voronoi_diagram &vd,
		const std::vector<Segment> &segments) {
	Faces_2_plus_1 ret;

	auto cell_contains_point = [&vd, &segments](const voronoi_diagram::cell_type *cell,
			const Point &point) {
		Segment segment = segments[cell->source_index()];
		return ( cell->contains_segment() && segment_has_endpoint(segment, point) )
			|| (cell->source_category() == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT
					&& segment.p0 == point)
			|| (cell->source_category() == boost::polygon::SOURCE_CATEGORY_SEGMENT_END_POINT
					&& segment.p1 == point);
	};

	auto edge_passes_through_point = [&vd, &segments, cell_contains_point](const voronoi_diagram::edge_type *edge,
			const Point &point) {
		return cell_contains_point(edge->cell(), point) && cell_contains_point(edge->twin()->cell(), point);
	};
				
	for (voronoi_diagram::const_cell_iterator cell_it = vd.cells().begin();
			cell_it != vd.cells().end(); cell_it++) {
		
		// cell info
		const voronoi_diagram::cell_type& cell = *cell_it;
		std::size_t cell_index = cell.source_index();
		// incident edge is cell.incident_edge();
		assert(!cell.is_degenerate());

		// all cells have an associated segment
		const Segment &segment = segments[cell_index];

		if (cell.contains_segment()) {
			// walk around the cell, find edge starting from segment.p1 or passing through it
			const voronoi_diagram::edge_type *edge = cell.incident_edge();
			
			std::cout << "\nsegment cell " << segment << "\n";

			for (;;) {
				if (edge_passes_through_point(edge, segment.p1) 
						&& !edge_passes_through_point(edge->next(), segment.p1)) {
					break;
				}
				edge = edge->next();
				assert(edge != cell.incident_edge());
			}

			std::cout << "starting edge: "; print_edge(edge); std::cout << "\n";
			std::cout << "next edge: "; print_edge(edge->next()); std::cout << "\n";
			std::cout << "segment.p1: " << segment.p1 << "\n";
			std::cout << "segment for edge: " << segments[edge->cell()->source_index()] << "\n";
			std::cout << "segment for twin: " << segments[edge->twin()->cell()->source_index()] << "\n";

			ret.faces.emplace_back();
			{
				Vector2d p(segment.p1.a, segment.p1.b);
				ret.faces.back().push_back(p);
				ret.heights[p] = 0.0;
			}
			do {
				if (edge->is_linear()) {
					Vector2d p(edge->vertex1()->x(), edge->vertex1()->y());
					ret.faces.back().push_back(p);
					ret.heights[p] = distance_to_segment(p, segment);
				} else {
					const voronoi_diagram::cell_type *twin_cell = edge->twin()->cell();
					assert(twin_cell->contains_point());
					Segment twin_segment = segments[twin_cell->source_index()];
					Point twin_point = 
						(twin_cell->source_category() == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT) ?
						twin_segment.p0 : twin_segment.p1;
					Vector2d v0(edge->vertex0()->x(), edge->vertex0()->y()),
							 v1(edge->vertex1()->x(), edge->vertex1()->y());
					std::vector<Vector2d> discr = discretize_arc(twin_point, segment, v1, v0);
					std::reverse(discr.begin(), discr.end());
					for (std::size_t k = 1; k < discr.size(); k++) {
						ret.faces.back().push_back(discr[k]);
						ret.heights[discr[k]] = distance_to_segment(discr[k], segment);
					}
				}
				edge = edge->next();
			} while (!edge_passes_through_point(edge, segment.p0));
			{
				Vector2d p(segment.p0.a, segment.p0.b);
				ret.faces.back().push_back(p);
				ret.heights[p] = 0.0;
			}
		} else {  // point cell
			const voronoi_diagram::edge_type *edge = cell.incident_edge();
			const Point point = (cell.source_category() == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT) ?
				segment.p0 : segment.p1;
			while (!( edge->is_secondary() && edge->prev()->is_secondary() )) {
				edge = edge->next();
			}

			std::cout << "\npoint cell" << point << "\n";
			std::cout << "starting edge: "; print_edge(edge); std::cout << "\n";
			std::cout << "twin stuff: " << segments[edge->twin()->cell()->source_index()].p0
				<< "\n";

			auto add_triangle = [&ret,&point](const Vector2d &v0, const Vector2d &v1) {
				ret.faces.emplace_back();

				Vector2d p(point.a, point.b);
				ret.faces.back().push_back(p);
				ret.heights[p] = 0.0;

				ret.faces.back().push_back(v0);
				ret.heights[v0] = distance_to_point(v0, point);

				ret.faces.back().push_back(v1);
				ret.heights[v1] = distance_to_point(v1, point);
			};

			if (edge->next()->next() != edge &&
					segments[edge->twin()->cell()->source_index()].p0 ==
					segments[edge->prev()->twin()->cell()->source_index()].p1) {
				// inner non-degenerate cell
				std::cout << "non-degenerate cell\n";
				for (;;) {
					edge = edge->next();
					if (edge->is_secondary()) {
						break;
					} else {
						Vector2d v0(edge->vertex0()->x(), edge->vertex0()->y()),
								 v1(edge->vertex1()->x(), edge->vertex1()->y());
						if (edge->is_curved()) {
							Segment twin_segment = segments[edge->twin()->cell()->source_index()];
							std::vector<Vector2d> discr = discretize_arc(point, twin_segment, v0, v1);
							for (std::size_t k = 1; k < discr.size(); k++) {
								add_triangle(discr[k-1], discr[k]);
							}
						} else {
							add_triangle(v0, v1);
						}
					}
				}
			} else {
				std::cout << "degenerate or outer cell\n";
			}
		}
	}

	return ret;
}

PolySet *voronoi_diagram_roof(const Polygon2d &poly)
{
	PolySet *hat = new PolySet(3);
	ClipperLib::Paths paths = ClipperUtils::fromPolygon2d(poly);
	std::vector<Segment> segments;

	for (auto path : paths) {
		auto prev = path.back();
		for (auto p : path) {
			segments.push_back(Segment(prev.X, prev.Y, p.X, p.Y));
			prev = p;
		}
	}
	
	std::cout << "Computing Voronoi\n" << std::flush;
	voronoi_diagram vd;
	boost::polygon::construct_voronoi(segments.begin(), segments.end(), &vd);
	std::cout << "Voronoi computed, computing inner faces\n" << std::flush;
	Faces_2_plus_1 inner_faces = vd_inner_faces(vd, segments);
	std::cout << "Inner faces computed\n";
	
	/*
	for (std::vector<Vector2d> face : inner_faces.faces) {
		assert(face.size() >= 3);

		// convex partition (or triangulation for a placeholder)
		Polygon2d face_poly;
		Outline2d outline;
		outline.vertices = face;
		face_poly.addOutline(outline);
		PolySet *tess = face_poly.tessellate();
		for (std::vector<Vector3d> triangle : tess->polygons) {
			Polygon floor, roof;
			for (Vector3d tv : triangle) {
				Vector2d v;
				v << tv[0], tv[1];
				auto d = ClipperUtils::CLIPPER_SCALE;
				floor.push_back({v[0] / d, v[1] / d, 0.0});
				assert(inner_faces.heights.find(v) != inner_faces.heights.end());
				roof.push_back({v[0] / d, v[1] / d, inner_faces.heights[v] / d});
			}
			std::reverse(floor.begin(), floor.end());  // floor has wrong orientation
			hat->append_poly(floor);
			hat->append_poly(roof);
		}
		delete tess;
	}
	*/

	for (std::vector<Vector2d> face : inner_faces.faces) {
		assert(face.size() >= 3);
		Polygon floor, roof;
		for (Vector2d v : face) {
			auto d = ClipperUtils::CLIPPER_SCALE;
			floor.push_back({v[0] / d, v[1] / d, 0.0});
			assert(inner_faces.heights.find(v) != inner_faces.heights.end());
			roof.push_back({v[0] / d, v[1] / d, inner_faces.heights[v] / d});
		}
		std::reverse(floor.begin(), floor.end());  // floor has wrong orientation
		hat->append_poly(floor);
		hat->append_poly(roof);
	}
	
	return hat;

}
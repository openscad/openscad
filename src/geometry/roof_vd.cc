// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#define _USE_MATH_DEFINES
#include <cmath>

#include <algorithm>
#include <map>
#include <boost/polygon/voronoi.hpp>
#include <boost/math/tools/roots.hpp>

#include "GeometryUtils.h"
#include "ClipperUtils.h"
#include "printutils.h"
#include "RoofNode.h"
#include "roof_vd.h"
#include "earcut.hpp"

#define RAISE_ROOF_EXCEPTION(message) \
  throw RoofNode::roof_exception((boost::format("%s line %d: %s") % __FILE__ % __LINE__ % (message)).str());

// implement "get" for Vector2d and ClipperLib::IntPoint for earcut
namespace mapbox {
namespace util {

template <>
struct nth<0, Vector2d> {
  inline static auto get(const Vector2d& v) {
    return v[0];
  }
};
template <>
struct nth<1, Vector2d> {
  inline static auto get(const Vector2d& v) {
    return v[1];
  }
};

template <>
struct nth<0, ClipperLib::IntPoint> {
  inline static auto get(const ClipperLib::IntPoint& t) {
    return t.X;
  }
};
template <>
struct nth<1, ClipperLib::IntPoint> {
  inline static auto get(const ClipperLib::IntPoint& t) {
    return t.Y;
  }
};

} // namespace util
} // namespace mapbox

namespace roof_vd {

typedef int32_t VD_int;

typedef ::boost::polygon::voronoi_diagram<double> voronoi_diagram;

struct Point {
  VD_int a;
  VD_int b;
  Point(VD_int x, VD_int y) : a(x), b(y) {}
  friend std::ostream& operator<<(std::ostream& os, const Point& point) {
    return os << "(" << point.a << ", " << point.b << ")";
  }
};

struct Segment {
  Point p0;
  Point p1;
  Segment(VD_int x1, VD_int y1, VD_int x2, VD_int y2) : p0(x1, y1), p1(x2, y2) {}
  friend std::ostream& operator<<(std::ostream& os, const Segment& segment) {
    return os << segment.p0 << " -- " << segment.p1;
  }
};

} // roof_vd

// pass our Point and Segment structures to boost::polygon
namespace boost {
namespace polygon {
template <>
struct geometry_concept<roof_vd::Point> {
  typedef point_concept type;
};
template <>
struct point_traits<roof_vd::Point> {
  typedef roof_vd::VD_int coordinate_type;

  static inline coordinate_type get(
    const roof_vd::Point& point, orientation_2d orient) {
    return (orient == HORIZONTAL) ? point.a : point.b;
  }
};
template <>
struct geometry_concept<roof_vd::Segment> {
  typedef segment_concept type;
};
template <>
struct segment_traits<roof_vd::Segment> {
  typedef roof_vd::VD_int coordinate_type;
  typedef roof_vd::Point point_type;

  static inline point_type get(const roof_vd::Segment& segment, direction_1d dir) {
    return dir.to_int() ? segment.p1 : segment.p0;
  }
};
}  // polygon
}  // boost


namespace roof_vd {

bool operator==(const Point& lhs, const Point& rhs)
{
  return lhs.a == rhs.a  &&  lhs.b == rhs.b;
}

bool operator==(const Segment& lhs, const Segment& rhs)
{
  return lhs.p0 == rhs.p0  &&  lhs.p1 == rhs.p1;
}

bool segment_has_endpoint(const Segment& segment, const Point& point) {
  return segment.p0 == point || segment.p1 == point;
}

double distance_to_segment(const Vector2d& vertex, const Segment& segment) {
  Vector2d segment_normal(-(segment.p1.b - segment.p0.b), segment.p1.a - segment.p0.a);
  segment_normal.normalize();
  Vector2d p0_to_vertex(vertex[0] - segment.p0.a, vertex[1] - segment.p0.b);
  return std::abs(segment_normal.dot(p0_to_vertex));
}

double distance_to_point(const Vector2d& vertex, const Point& point) {
  Vector2d point_to_vertex(vertex[0] - point.a, vertex[1] - point.b);
  return point_to_vertex.norm();
}

std::vector<Vector2d> discretize_arc(const Point& point, const Segment& segment,
                                     const Vector2d& v0, const Vector2d& v1,
                                     double fa, double fs)
{
  std::vector<Vector2d> ret;

  const double fa_rad = M_PI / 180.0 * fa;

  const Vector2d p(point.a, point.b);
  const Vector2d p0(segment.p0.a, segment.p0.b);
  const Vector2d p1(segment.p1.a, segment.p1.b);
  const Vector2d p0_to_p1_norm = (p1 - p0).normalized();

  const Vector2d projected_point = p0 + p0_to_p1_norm * p0_to_p1_norm.dot(p - p0);

  const double point_distance = (p - projected_point).norm();

  if (!(point_distance > 0)) {
    RAISE_ROOF_EXCEPTION("error in parabolic arc discretization");
  }

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
    RAISE_ROOF_EXCEPTION("error in parabolic arc discretization");
  }

  // in transformed coordinates the parabola has equation y = ...
  auto y = [point_distance](double x) {
      return (x * x - point_distance * point_distance) / (2 * point_distance);
    };
  auto y_prime = [point_distance](double x) {
      return x / point_distance;
    };
  // angle of the tangent to the parabola at x
  auto angle = [y_prime](double x){
      return std::atan2(y_prime(x), 1.0);
    };
  // .. and its inverse
  auto angle_inv = [point_distance, y_prime](double a){
      return point_distance * std::tan(a);
    };
  // squared length of segment
  // arch length on the parabola between the vertex and the point
  // (oriented, negative on the left, positive on the right)
  auto arc_length = [point_distance](double x){
      const double d = point_distance;
      return 0.5 * (x * sqrt(x * x / (d * d) + 1.0) + d * asinh(x / d));
    };
  // derivative of arc_length
  auto arc_length_prime = [point_distance](double x){
      const double d = point_distance;
      return sqrt(x * x / (d * d) + 1.0);
    };
  // inverse of arc_length, no explicit formula sadly
  auto arc_length_inv = [arc_length, arc_length_prime, point_distance](double t){
      const double d = point_distance;
      if (t == 0) {
        return double(0);
      }
      double x_guess = ((t > 0) ? 1 : -1) * std::sqrt(2 * d * (-d + std::sqrt(d * d + t * t))),
             x_min = (t > 0) ? (x_guess / 2) : x_guess,
             x_max = (t < 0) ? (x_guess / 2) : x_guess;
      const int digits = std::numeric_limits<double>::digits;
      double x;
      try {
        // try Newton-Raphson
        auto feed = [arc_length, arc_length_prime, t](double x) {
            return std::make_pair(arc_length(x) - t, arc_length_prime(x));
          };
        int get_digits = static_cast<int>(digits * 0.6);
        const boost::uintmax_t maxit = 4242;
        boost::uintmax_t it = maxit;
        x = boost::math::tools::newton_raphson_iterate(feed, x_guess, x_min, x_max, get_digits, it);
      } catch (...) {
        // fall back to bisection
        auto feed = [arc_length, t](double x) {
            return arc_length(x) - t;
          };
        int get_digits = static_cast<int>(digits - 3);
        boost::math::tools::eps_tolerance<double> tol(get_digits);
        try {
          auto xxx = boost::math::tools::bisect(feed, x_min, x_max, tol);
          x = (xxx.first + xxx.second) / 2;
        } catch (...) {
          RAISE_ROOF_EXCEPTION("error in parabolic arc discretization");
        }
      }
      return x;
    };

  double arc_length_0 = arc_length(transformed_v0_x),
         arc_length_1 = arc_length(transformed_v1_x);
  // number of points if we discretize according to fs
  int segments_fs = (fs == 0.0) ? 1 : std::ceil((arc_length_1 - arc_length_0) / fs);

  double angle_0 = angle(transformed_v0_x),
         angle_1 = angle(transformed_v1_x);
  // number of points if we discretize according to fa
  int segments_fa = std::ceil((angle_1 - angle_0) / fa_rad);

  // make a choice and discretize
  std::vector<double> transformed_points_x;
  if (fs > 0 && segments_fs < segments_fa) {
    // fs wins
    transformed_points_x.reserve(segments_fs + 1);
    transformed_points_x.push_back(transformed_v0_x);
    for (int k = 1; k < segments_fs; k++) {
      double a = (arc_length_0 * (segments_fs - k) + arc_length_1 * k) / segments_fs;
      transformed_points_x.push_back(arc_length_inv(a));
    }
    transformed_points_x.push_back(transformed_v1_x);
  } else {
    // fa wins
    transformed_points_x.reserve(segments_fa + 1);
    transformed_points_x.push_back(transformed_v0_x);
    for (int k = 1; k < segments_fa; k++) {
      double a = (angle_0 * (segments_fa - k) + angle_1 * k) / segments_fa;
      transformed_points_x.push_back(angle_inv(a));
    }
    transformed_points_x.push_back(transformed_v1_x);
  }

  // assemble the discretized vector
  for (auto x : transformed_points_x) {
    if (x == transformed_v0_x) {
      ret.push_back(v0);
    } else if (x == transformed_v1_x) {
      ret.push_back(v1);
    } else {
      ret.push_back(p + Ai * Vector2d(x, y(x)));
    }
  }

  return ret;
}

// a structure that saves 2d faces and heights of vertices
struct Faces_2_plus_1 {
  struct Vector2d_comp {
    bool operator()(const Vector2d& lhs, const Vector2d& rhs) const {
      return (lhs[0] < rhs[0]) || (lhs[0] == rhs[0] && lhs[1] < rhs[1]);
    }
  };
  std::vector<std::vector<Vector2d>> faces;
  std::map<Vector2d, double, Vector2d_comp> heights;
};

Faces_2_plus_1 vd_inner_faces(const voronoi_diagram& vd,
                              const std::vector<Segment>& segments,
                              double fa, double fs) {
  Faces_2_plus_1 ret;

  auto cell_contains_boundary_point = [&vd, &segments](const voronoi_diagram::cell_type *cell,
                                                       const Point& point) {
      Segment segment = segments[cell->source_index()];
      return (cell->contains_segment() && segment_has_endpoint(segment, point) )
             || (cell->source_category() == ::boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT
                 && segment.p0 == point)
             || (cell->source_category() == ::boost::polygon::SOURCE_CATEGORY_SEGMENT_END_POINT
                 && segment.p1 == point);
    };

  for (voronoi_diagram::const_cell_iterator cell = vd.cells().begin(); cell != vd.cells().end(); cell++) {

    std::size_t cell_index = cell->source_index();
    if (cell->is_degenerate()) {
      RAISE_ROOF_EXCEPTION("Voronoi error");
    }
    const Segment& segment = segments[cell_index];

    if (cell->contains_segment()) {
      // walk around the cell, find edge starting from segment.p1 or passing through it
      const voronoi_diagram::edge_type *edge = cell->incident_edge();
      for (;;) {
        if (cell_contains_boundary_point(edge->twin()->cell(), segment.p1)
            && !cell_contains_boundary_point(edge->next()->twin()->cell(), segment.p1)) {
          break;
        }
        edge = edge->next();
        if (edge == cell->incident_edge()) {
          RAISE_ROOF_EXCEPTION("Voronoi error");
        }
      }
      // add all inside edges
      ret.faces.emplace_back();
      {
        Vector2d p(segment.p1.a, segment.p1.b);
        ret.faces.back().push_back(p);
        ret.heights[p] = 0.0;
      }
      do {
        if (edge->is_linear()) { // linear edge is simple
          Vector2d p(edge->vertex1()->x(), edge->vertex1()->y());
          ret.faces.back().push_back(p);
          ret.heights[p] = distance_to_segment(p, segment);
        } else { // discretize a parabolic edge
          const voronoi_diagram::cell_type *twin_cell = edge->twin()->cell();
          if (!(twin_cell->contains_point())) {
            RAISE_ROOF_EXCEPTION("Voronoi error");
          }
          Segment twin_segment = segments[twin_cell->source_index()];
          Point twin_point =
            (twin_cell->source_category() == ::boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT) ?
            twin_segment.p0 : twin_segment.p1;
          Vector2d v0(edge->vertex0()->x(), edge->vertex0()->y()),
          v1(edge->vertex1()->x(), edge->vertex1()->y());
          std::vector<Vector2d> discr = discretize_arc(twin_point, segment, v1, v0, fa, fs);
          std::reverse(discr.begin(), discr.end());
          for (std::size_t k = 1; k < discr.size(); k++) {
            ret.faces.back().push_back(discr[k]);
            ret.heights[discr[k]] = distance_to_segment(discr[k], segment);
          }
        }
        edge = edge->next();
      } while (!cell_contains_boundary_point(edge->twin()->cell(), segment.p0));
      {
        Vector2d p(segment.p0.a, segment.p0.b);
        ret.faces.back().push_back(p);
        ret.heights[p] = 0.0;
      }
    } else { // point cell
      const voronoi_diagram::edge_type *edge = cell->incident_edge();
      const Point point = (cell->source_category() == ::boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT) ?
        segment.p0 : segment.p1;
      while (!(edge->is_secondary() && edge->prev()->is_secondary() )) {
        edge = edge->next();
        if (edge == cell->incident_edge()) {
          RAISE_ROOF_EXCEPTION("Voronoi error");
        }
      }

      auto add_triangle = [&ret, &point](const Vector2d& v0, const Vector2d& v1) {
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
        for (;;) {
          edge = edge->next();
          if (edge->is_secondary()) {
            break;
          } else {
            Vector2d v0(edge->vertex0()->x(), edge->vertex0()->y()),
            v1(edge->vertex1()->x(), edge->vertex1()->y());
            if (edge->is_curved()) {
              Segment twin_segment = segments[edge->twin()->cell()->source_index()];
              std::vector<Vector2d> discr = discretize_arc(point, twin_segment, v0, v1, fa, fs);
              for (std::size_t k = 1; k < discr.size(); k++) {
                add_triangle(discr[k - 1], discr[k]);
              }
            } else {
              add_triangle(v0, v1);
            }
          }
        }
      }
    }
  }
  return ret;
}

PolySet *voronoi_diagram_roof(const Polygon2d& poly, double fa, double fs)
{
  PolySet *hat = new PolySet(3);

  try {

    // input data for voronoi diagram is 32 bit integers
    int scale_pow2 = ClipperUtils::getScalePow2(poly.getBoundingBox(), 32);
    double scale = std::ldexp(1.0, scale_pow2);

    ClipperLib::Paths paths = ClipperUtils::fromPolygon2d(poly, scale_pow2);
    // sanitize is important e.g. when after converting to 32 bit integers we have double points
    ClipperLib::PolyTreeToPaths(ClipperUtils::sanitize(paths), paths);
    std::vector<Segment> segments;

    for (auto path : paths) {
      auto prev = path.back();
      for (auto p : path) {
        segments.push_back(Segment(prev.X, prev.Y, p.X, p.Y));
        prev = p;
      }
    }

    voronoi_diagram vd;
    ::boost::polygon::construct_voronoi(segments.begin(), segments.end(), &vd);
    Faces_2_plus_1 inner_faces = vd_inner_faces(vd, segments, fa, scale * fs);

    // tessellate roof and add triangles to hat
    for (std::vector<Vector2d> face : inner_faces.faces) {
      if (!(face.size() >= 3)) {
        RAISE_ROOF_EXCEPTION("Voronoi error");
      }
      std::vector<std::vector<Vector2d>> face_array = { face };
      const std::vector<size_t> indices = mapbox::earcut<size_t>(face_array);
      for (size_t i = 0; i < indices.size(); i += 3) {
        std::vector<Vector3d> triangle(3);
        for (size_t k = 0; k < 3; k++) {
          triangle[k][0] = face[indices[i + k]][0] / scale;
          triangle[k][1] = face[indices[i + k]][1] / scale;
          triangle[k][2] = inner_faces.heights[face[indices[i + k]]] / scale;
        }
        hat->append_poly(triangle);
      }
    }

    // tessellate floor and add triangles to hat
    {
      // poly has to go through clipper just as it does for the roof
      // because this may change coordinates
      const std::vector<size_t> indices = mapbox::earcut<size_t>(paths);
      std::vector<Vector3d> vertices;
      for (auto path : paths) {
        for (auto p : path) {
          vertices.push_back({p.X / scale, p.Y / scale, 0.0});
        }
      }
      for (size_t i = 0; i < indices.size(); i += 3) {
        std::vector<Vector3d> triangle(3);
        for (size_t k = 0; k < 3; k++) {
          // floor has reverse orientation hence 2-k
          triangle[2 - k] = vertices[indices[i + k]];
        }
        hat->append_poly(triangle);
      }
    }
  } catch (RoofNode::roof_exception& e) {
    delete hat;
    throw e;
  }

  return hat;
}

} // roof_vd

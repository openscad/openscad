// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#include "geometry/roof_vd.h"

#include <algorithm>
#include <boost/math/tools/roots.hpp>
#include <limits>
#include <boost/polygon/voronoi.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <vector>

#include "core/CurveDiscretizer.h"
#include "core/RoofNode.h"
#include "geometry/ClipperUtils.h"
#include "geometry/GeometryUtils.h"
#include "geometry/PolySetBuilder.h"
#include "geometry/Polygon2d.h"
#include "geometry/linalg.h"

#define RAISE_ROOF_EXCEPTION(message) \
  throw RoofNode::roof_exception(     \
    (boost::format("%s line %d: %s") % __FILE__ % __LINE__ % (message)).str())

#define SAFE_VERTEX_MSG(edge, numeral, msg) \
  (edge->vertex##numeral() == nullptr ? (RAISE_ROOF_EXCEPTION(msg), nullptr) : edge->vertex##numeral())

#define SAFE_VERTEX(edge, numeral)                                                                      \
  SAFE_VERTEX_MSG(edge, numeral,                                                                        \
                  "Voronoi error: there's probably a zero-thickness wall causing a discontinuity. For " \
                  "now, you must shift things so this can't occur")

namespace roof_vd {

using VD_int = int32_t;

using voronoi_diagram = ::boost::polygon::voronoi_diagram<double>;

struct Point {
  VD_int a;
  VD_int b;
  Point(VD_int x, VD_int y) : a(x), b(y) {}
  friend std::ostream& operator<<(std::ostream& os, const Point& point)
  {
    return os << "(" << point.a << ", " << point.b << ")";
  }
};

struct Segment {
  Point p0;
  Point p1;
  Segment(VD_int x1, VD_int y1, VD_int x2, VD_int y2) : p0(x1, y1), p1(x2, y2) {}
  friend std::ostream& operator<<(std::ostream& os, const Segment& segment)
  {
    return os << segment.p0 << " -- " << segment.p1;
  }
};

}  // namespace roof_vd

// pass our Point and Segment structures to boost::polygon
namespace boost::polygon {
template <>
struct geometry_concept<roof_vd::Point> {
  using type = point_concept;
};
template <>
struct point_traits<roof_vd::Point> {
  using coordinate_type = roof_vd::VD_int;

  static inline coordinate_type get(const roof_vd::Point& point, const orientation_2d& orient)
  {
    return (orient == HORIZONTAL) ? point.a : point.b;
  }
};
template <>
struct geometry_concept<roof_vd::Segment> {
  using type = segment_concept;
};
template <>
struct segment_traits<roof_vd::Segment> {
  using coordinate_type = roof_vd::VD_int;
  using point_type = roof_vd::Point;

  static inline point_type get(const roof_vd::Segment& segment, const direction_1d& dir)
  {
    return dir.to_int() ? segment.p1 : segment.p0;
  }
};
}  // namespace boost::polygon

namespace roof_vd {

bool operator==(const Point& lhs, const Point& rhs)
{
  return lhs.a == rhs.a && lhs.b == rhs.b;
}

bool operator==(const Segment& lhs, const Segment& rhs)
{
  return lhs.p0 == rhs.p0 && lhs.p1 == rhs.p1;
}

bool segment_has_endpoint(const Segment& segment, const Point& point)
{
  return segment.p0 == point || segment.p1 == point;
}

double distance_to_segment(const Vector2d& vertex, const Segment& segment)
{
  Vector2d segment_normal(-(segment.p1.b - segment.p0.b), segment.p1.a - segment.p0.a);
  segment_normal.normalize();
  Vector2d p0_to_vertex(vertex[0] - segment.p0.a, vertex[1] - segment.p0.b);
  return std::abs(segment_normal.dot(p0_to_vertex));
}

double distance_to_point(const Vector2d& vertex, const Point& point)
{
  Vector2d point_to_vertex(vertex[0] - point.a, vertex[1] - point.b);
  return point_to_vertex.norm();
}

std::vector<Vector2d> discretize_arc(const Point& point, const Segment& segment, const Vector2d& v0,
                                     const Vector2d& v1, const RoofDiscretizer& discretizer)
{
  std::vector<Vector2d> ret;

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
  //     a_point -> A(a_point - point)
  Eigen::Matrix2d A, Ai;
  Ai << point_direction.y(), point_direction.x(), -point_direction.x(), point_direction.y();
  A = Ai.inverse();

  // x coordinates of source and target
  const double transformed_v0_x = (A * (v0 - p))[0];
  const double transformed_v1_x = (A * (v1 - p))[0];
  if (!(transformed_v0_x < transformed_v1_x)) {
    RAISE_ROOF_EXCEPTION("error in parabolic arc discretization");
  }

  // in transformed coordinates the parabola has equation y = (x^2 - point_distance^2) / (2
  // point_distance)
  auto y = [point_distance](double x) {
    return (x * x - point_distance * point_distance) / (2 * point_distance);
  };

  auto angle = [point_distance](double x) { return std::atan(x / point_distance); };
  auto angle_inv = [point_distance](double a) { return point_distance * std::tan(a); };
  auto arc_length = [point_distance](double x) {
    const double d = point_distance;
    return 0.5 * (x * std::sqrt(x * x / (d * d) + 1.0) + d * std::asinh(x / d));
  };
  auto arc_length_prime = [point_distance](double x) {
    const double d = point_distance;
    return std::sqrt(x * x / (d * d) + 1.0);
  };
  // inverse of arc_length, solved numerically
  auto arc_length_inv = [point_distance, arc_length, arc_length_prime](double t) {
    const double d = point_distance;
    if (t == 0.0) return 0.0;
    const double bound =
      std::sqrt(2.0 * d * std::abs(t)) * (1.0 + 16.0 * std::numeric_limits<double>::epsilon());
    if (!(bound > 0.0)) return 0.0;  // t is far too small to move x at all
    const double x_min = (t > 0.0) ? 0.0 : -bound;
    const double x_max = (t > 0.0) ? bound : 0.0;
    const double x_guess = std::copysign(std::sqrt(2.0 * d * (std::hypot(d, t) - d)), t);
    constexpr int digits = std::numeric_limits<double>::digits;
    try {
      auto newton = [arc_length, arc_length_prime, t](double x) {
        return std::make_pair(arc_length(x) - t, arc_length_prime(x));
      };
      std::uintmax_t max_iter = 100;
      return boost::math::tools::newton_raphson_iterate(newton, x_guess, x_min, x_max,
                                                        static_cast<int>(digits * 0.6), max_iter);
    } catch (...) {
      // fall back to bisection
      auto bisect = [arc_length, t](double x) { return arc_length(x) - t; };
      try {
        const auto bracket = boost::math::tools::bisect(
          bisect, x_min, x_max, boost::math::tools::eps_tolerance<double>(digits - 3));
        return (bracket.first + bracket.second) / 2.0;
      } catch (...) {
        RAISE_ROOF_EXCEPTION("error in parabolic arc discretization");
      }
    }
  };

  const double angle_0 = angle(transformed_v0_x), angle_1 = angle(transformed_v1_x);
  const double arc_length_0 = arc_length(transformed_v0_x), arc_length_1 = arc_length(transformed_v1_x);
  const double max_segment_length = discretizer.maxSegmentLength();
  const double segments_fa = (angle_1 - angle_0) / discretizer.maxAngle();
  const double segments_fs =
    (max_segment_length > 0.0) ? (arc_length_1 - arc_length_0) / max_segment_length : 1.0;

  const bool by_arc_length = (max_segment_length > 0.0) && (segments_fs < segments_fa);
  const double wanted = by_arc_length ? segments_fs : segments_fa;
  if (!(wanted <= static_cast<double>(std::numeric_limits<int>::max()))) {
    RAISE_ROOF_EXCEPTION("too many segments to discretize, try a larger $fa or $fs");
  }
  const int segments = static_cast<int>(std::max(1.0, std::ceil(wanted)));
  std::vector<double> transformed_points_x;
  transformed_points_x.reserve(segments + 1);
  transformed_points_x.push_back(transformed_v0_x);
  for (int k = 1; k < segments; k++) {
    if (by_arc_length) {
      const double arc_length_k = (arc_length_0 * (segments - k) + arc_length_1 * k) / segments;
      transformed_points_x.push_back(arc_length_inv(arc_length_k));
    } else {
      const double angle_k = (angle_0 * (segments - k) + angle_1 * k) / segments;
      transformed_points_x.push_back(angle_inv(angle_k));
    }
  }
  transformed_points_x.push_back(transformed_v1_x);

  for (auto x : transformed_points_x) {
    if (x == transformed_v0_x) {
      ret.push_back(v0);
    } else if (x == transformed_v1_x) {
      ret.push_back(v1);
    } else {
      ret.emplace_back(p + Ai * Vector2d(x, y(x)));
    }
  }

  return ret;
}

// a structure that saves 2d faces and heights of vertices
struct Faces_2_plus_1 {
  struct Vector2d_comp {
    bool operator()(const Vector2d& lhs, const Vector2d& rhs) const
    {
      return (lhs[0] < rhs[0]) || (lhs[0] == rhs[0] && lhs[1] < rhs[1]);
    }
  };
  std::vector<std::vector<Vector2d>> faces;
  std::map<Vector2d, double, Vector2d_comp> heights;
};

Faces_2_plus_1 vd_inner_faces(const voronoi_diagram& vd, const std::vector<Segment>& segments,
                              const RoofDiscretizer& discretizer)
{
  Faces_2_plus_1 ret;

  auto cell_contains_boundary_point = [&segments](const voronoi_diagram::cell_type *cell,
                                                  const Point& point) {
    Segment segment = segments[cell->source_index()];
    return (cell->contains_segment() && segment_has_endpoint(segment, point)) ||
           (cell->source_category() == ::boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT &&
            segment.p0 == point) ||
           (cell->source_category() == ::boost::polygon::SOURCE_CATEGORY_SEGMENT_END_POINT &&
            segment.p1 == point);
  };

  for (const auto& cell : vd.cells()) {
    std::size_t cell_index = cell.source_index();
    if (cell.is_degenerate()) {
      RAISE_ROOF_EXCEPTION("Voronoi error");
    }
    const Segment& segment = segments[cell_index];

    if (cell.contains_segment()) {
      // walk around the cell, find edge starting from segment.p1 or passing through it
      const voronoi_diagram::edge_type *edge = cell.incident_edge();
      for (;;) {
        if (cell_contains_boundary_point(edge->twin()->cell(), segment.p1) &&
            !cell_contains_boundary_point(edge->next()->twin()->cell(), segment.p1)) {
          break;
        }
        edge = edge->next();
        if (edge == cell.incident_edge()) {
          RAISE_ROOF_EXCEPTION("Voronoi error");
        }
      }

      // add all inside edges
      ret.faces.emplace_back();  // Puts a new empty vector<Vector2d> at the end of ret.faces.
      {
        Vector2d p(segment.p1.a, segment.p1.b);
        ret.faces.back().push_back(p);
        ret.heights[p] = 0.0;
      }
      do {
        if (edge->is_linear()) {  // linear edge is simple
          auto vtx1 = SAFE_VERTEX(edge, 1);
          Vector2d p(vtx1->x(), vtx1->y());
          ret.faces.back().push_back(p);
          ret.heights[p] = distance_to_segment(p, segment);
        } else {  // discretize a parabolic edge
          const voronoi_diagram::cell_type *twin_cell = edge->twin()->cell();
          if (!(twin_cell->contains_point())) {
            RAISE_ROOF_EXCEPTION("Voronoi error");
          }
          Segment twin_segment = segments[twin_cell->source_index()];
          Point twin_point =
            (twin_cell->source_category() == ::boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT)
              ? twin_segment.p0
              : twin_segment.p1;
          auto vtx0 = SAFE_VERTEX(edge, 0), vtx1 = SAFE_VERTEX(edge, 1);
          Vector2d v0(vtx0->x(), vtx0->y()), v1(vtx1->x(), vtx1->y());

          std::vector<Vector2d> discr = discretize_arc(twin_point, segment, v1, v0, discretizer);
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
    } else {  // point cell
      const voronoi_diagram::edge_type *edge = cell.incident_edge();
      const Point point =
        (cell.source_category() == ::boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT) ? segment.p0
                                                                                          : segment.p1;
      while (!(edge->is_secondary() && edge->prev()->is_secondary())) {
        edge = edge->next();
        if (edge == cell.incident_edge()) {
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

      if (edge->next()->next() != edge && segments[edge->twin()->cell()->source_index()].p0 ==
                                            segments[edge->prev()->twin()->cell()->source_index()].p1) {
        // inner non-degenerate cell
        for (;;) {
          edge = edge->next();
          if (edge->is_secondary()) {
            break;
          } else {
            auto vtx0 = SAFE_VERTEX(edge, 0), vtx1 = SAFE_VERTEX(edge, 1);
            Vector2d v0(vtx0->x(), vtx0->y()), v1(vtx1->x(), vtx1->y());
            if (edge->is_curved()) {
              Segment twin_segment = segments[edge->twin()->cell()->source_index()];
              std::vector<Vector2d> discr = discretize_arc(point, twin_segment, v0, v1, discretizer);
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

std::unique_ptr<PolySet> voronoi_diagram_roof(const Polygon2d& poly, const CurveDiscretizer& discretizer)
{
  PolySetBuilder hatbuilder = PolySetBuilder();

  try {
    // input data for voronoi diagram is 32 bit integers
    // FIXME: Why does this need to be 32 bits? The default we use elsewhere is
    // scaleBitsFromPrecision(DEFAULT_PRECISION) which is 10^8.
    const int scale_bits = ClipperUtils::scaleBitsFromBounds(poly.getBoundingBox(), 32);
    const double scale = std::ldexp(1.0, scale_bits);

    Clipper2Lib::Paths64 paths = ClipperUtils::fromPolygon2d(poly, scale_bits);
    // sanitize is important e.g. when after converting to 32 bit integers we have double points
    paths = Clipper2Lib::PolyTreeToPaths64(*ClipperUtils::sanitize(paths));
    std::vector<Segment> segments;

    for (auto path : paths) {
      auto prev = path.back();
      for (auto p : path) {
        segments.emplace_back(prev.x, prev.y, p.x, p.y);
        prev = p;
      }
    }

    voronoi_diagram vd;
    ::boost::polygon::construct_voronoi(segments.begin(), segments.end(), &vd);
    Faces_2_plus_1 inner_faces = vd_inner_faces(vd, segments, RoofDiscretizer(discretizer, scale));

    // roof
    for (const std::vector<Vector2d>& face : inner_faces.faces) {
      if (!(face.size() >= 3)) {
        RAISE_ROOF_EXCEPTION("Voronoi error");
      }
      // convex partition (actually a triangulation - maybe do a proper convex partition later)
      Polygon2d face_poly;
      Outline2d outline;
      outline.vertices.assign(face.begin(), face.end());
      face_poly.addOutline(outline);
      auto tess = face_poly.tessellate();
      for (const IndexedFace& triangle : tess->indices) {
        std::vector<int> roof;
        for (int tvind : triangle) {
          Vector3d tv = tess->vertices[tvind];
          Vector2d v;
          v << tv[0], tv[1];
          if (!(inner_faces.heights.find(v) != inner_faces.heights.end())) {
            RAISE_ROOF_EXCEPTION("Voronoi error");
          }
          roof.push_back(hatbuilder.vertexIndex(
            Vector3d(v[0] / scale, v[1] / scale, inner_faces.heights[v] / scale)));
        }
        hatbuilder.appendPolygon(roof);
      }
    }

    // floor
    {
      // poly has to go through clipper just as it does for the roof
      // because this may change coordinates
      Polygon2d poly_floor;
      for (const auto& path : paths) {
        Outline2d o;
        for (auto p : path) {
          o.vertices.push_back({p.x / scale, p.y / scale});
        }
        poly_floor.addOutline(o);
      }
      auto tess = poly_floor.tessellate();
      for (const IndexedFace& triangle : tess->indices) {
        std::vector<int> floor;
        for (const int tv : triangle) {
          floor.push_back(hatbuilder.vertexIndex(tess->vertices[tv]));
        }
        // floor has reverse orientation
        std::reverse(floor.begin(), floor.end());
        hatbuilder.appendPolygon(floor);
      }
    }
  } catch (RoofNode::roof_exception& e) {
    throw;
  }

  return hatbuilder.build();
}

}  // namespace roof_vd

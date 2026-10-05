/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "core/primitives.h"

#include <algorithm>
#include <boost/assign/std/vector.hpp>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/Builtins.h"
#include "core/Children.h"
#include "core/ModuleInstantiation.h"
#include "core/Parameters.h"
#include "core/Value.h"
#include "core/module.h"
#include "core/node.h"
#include "geometry/Geometry.h"
#include "geometry/PolySet.h"
#include "geometry/Polygon2d.h"
#include "geometry/linalg.h"
#include "utils/calc.h"
#include "utils/degree_trig.h"
#include "utils/printutils.h"

using namespace boost::assign;  // bring 'operator+=()' into scope

template <class InsertIterator>
static void generate_circle(InsertIterator iter, double r, double z, int fragments)
{
  for (int i = 0; i < fragments; ++i) {
    double phi = (360.0 * i) / fragments;
    *(iter++) = {r * cos_degrees(phi), r * sin_degrees(phi), z};
  }
}

std::unique_ptr<const Geometry> CubeNode::createGeometry() const
{
  if (this->x <= 0 || !std::isfinite(this->x) || this->y <= 0 || !std::isfinite(this->y) ||
      this->z <= 0 || !std::isfinite(this->z)) {
    return PolySet::createEmpty();
  }

  double x1, x2, y1, y2, z1, z2;
  if (this->center) {
    x1 = -this->x / 2;
    x2 = +this->x / 2;
    y1 = -this->y / 2;
    y2 = +this->y / 2;
    z1 = -this->z / 2;
    z2 = +this->z / 2;
  } else {
    x1 = y1 = z1 = 0;
    x2 = this->x;
    y2 = this->y;
    z2 = this->z;
  }
  auto ps = std::make_unique<PolySet>(3, /*convex*/ true);
  for (int i = 0; i < 8; i++) {
    ps->vertices.emplace_back(i & 1 ? x2 : x1, i & 2 ? y2 : y1, i & 4 ? z2 : z1);
  }
  ps->indices = {
    {4, 5, 7, 6},  // top
    {2, 3, 1, 0},  // bottom
    {0, 1, 5, 4},  // front
    {1, 3, 7, 5},  // right
    {3, 2, 6, 7},  // back
    {2, 0, 4, 6},  // left
  };

  return ps;
}

static std::shared_ptr<AbstractNode> builtin_cube(const ModuleInstantiation *inst, Arguments arguments)
{
  auto node = std::make_shared<CubeNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"size", "center"});

  const auto& size = parameters["size"];
  if (size.isDefined()) {
    bool converted = false;
    converted |= size.getDouble(node->x);
    converted |= size.getDouble(node->y);
    converted |= size.getDouble(node->z);
    converted |= size.getVec3(node->x, node->y, node->z);
    if (!converted) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unable to convert cube(size=%1$s, ...) parameter to a number or a vec3 of numbers",
          size.toEchoStringNoThrow());
    } else if (OpenSCAD::rangeCheck) {
      bool ok = (node->x > 0) && (node->y > 0) && (node->z > 0);
      ok &= std::isfinite(node->x) && std::isfinite(node->y) && std::isfinite(node->z);
      if (!ok) {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "cube(size=%1$s, ...)",
            size.toEchoStringNoThrow());
      }
    }
  }
  if (parameters["center"].type() == Value::Type::BOOL) {
    node->center = parameters["center"].toBool();
  }

  return node;
}

std::string SphereNode::toString() const
{
  std::ostringstream stream;
  stream << "sphere(" << discretizer << ", r = " << r << ", style = \"" << style << "\")";
  return stream.str();
}

/*
 * Octahedral sphere tessellation (style = "octa").
 *
 * Start from an octahedron whose six vertices are on the axis poles and
 * subdivide each of its eight triangular faces into n*n triangles, with
 * n = ceil(fragments / 4). Each of the three great circles in the coordinate
 * planes then is a regular polygon with 4*n segments whose vertices are
 * exactly those of circle()/cylinder() with the same number of fragments, so
 * the sphere fits cylinders on any axis without slivers. All vertices are on
 * the sphere, there is a vertex on each of the six poles and the mesh has the
 * full symmetry of the octahedron.
 *
 * Interior vertices are placed using the algorithm posted by Bram Cohen in
 * https://github.com/openscad/openscad/pull/6100#issuecomment-3229898933:
 * the vertex with barycentric index (i, j, k), i + j + k = n, of the octant
 * face is where the great-circle arcs joining the equally spaced edge points
 * at "constant i", "constant j" and "constant k" meet. The three arcs do not
 * pass exactly through one point, so the three pairwise intersections are
 * averaged and projected back onto the sphere. This keeps the three-fold
 * symmetry of each face and makes the edge lengths very even.
 *
 * Vertices are stored in rings around the z axis: ring 0 is the +z pole,
 * ring t (1 <= t <= n) has 4*t vertices, t per quadrant, starting on the
 * meridian at the +x side of the quadrant and going counterclockwise as seen
 * from +z. Ring n is the equator and rings n+1..2n mirror rings n-1..0.
 * Ring t starts at vertex index 1 + 2*t*(t-1), so the vertex at quadrant q
 * (0..3) and position s (0..t-1) is at 1 + 2*t*(t-1) + q*t + s; ring_start[]
 * below holds these offsets. Seen from +z (x right, y up) for n = 3 the top
 * hemisphere is
 *
 *                        16
 *                 17            15             ring 3 (equator): 13..24
 *                        7
 *            18     8         6     14         ring 2: 5..12
 *                        2
 *       19     9     3   0   1     5     13    ring 1: 1..4, pole: 0
 *                        4
 *            20    10        12     24
 *                       11
 *                 21            23
 *                        22
 *
 * Between ring t and ring t+1 each quadrant has t+1 triangles with their apex
 * on ring t and t triangles with their apex on ring t+1, see stitch() below.
 */
std::unique_ptr<PolySet> SphereNode::createGeometryOcta(int num_fragments) const
{
  const int n = std::max(1, (num_fragments + 3) / 4);  // subdivisions per octahedron edge
  const int fragments = 4 * n;                         // segments per equator

  // Angle of the m-th of the n equally spaced points along an octahedron edge,
  // computed as in generate_circle() so shared vertices are bit-identical.
  std::vector<double> cos_table(n + 1), sin_table(n + 1);
  for (int m = 0; m <= n; ++m) {
    const double phi = (360.0 * m) / fragments;
    cos_table[m] = cos_degrees(phi);
    sin_table[m] = sin_degrees(phi);
  }

  // Where the great circles through (a, b) and (c, d) intersect (on the side
  // of the sphere where the arcs are).
  const auto arc_intersection = [](const Vector3d& a, const Vector3d& b, const Vector3d& c,
                                   const Vector3d& d) -> Vector3d {
    return a.cross(b).cross(c.cross(d)).normalized();
  };

  // Interior points of the first octant on the unit sphere, indexed like ring t,
  // quadrant 0: octant[t][s] has barycentric index i = t - s, j = s, k = n - t.
  // The other seven octants are exact rotations/mirrors of these. Points on the
  // octahedron edges are not in this table, they are generated below.
  std::vector<std::vector<Vector3d>> octant(n);
  for (int t = 2; t < n; ++t) {
    octant[t].resize(t);
    const int k = n - t;
    for (int s = 1; s < t; ++s) {
      const int i = t - s;
      const int j = s;
      // Arc at constant k: joins the k-th point of the xz edge and of the yz edge.
      const Vector3d xz_k(cos_table[k], 0, sin_table[k]);
      const Vector3d yz_k(0, cos_table[k], sin_table[k]);
      // Arc at constant j: joins the j-th point of the xy edge and the (n-j)-th of the yz edge.
      const Vector3d xy_j(cos_table[j], sin_table[j], 0);
      const Vector3d yz_j(0, cos_table[n - j], sin_table[n - j]);
      // Arc at constant i: joins the (n-i)-th point of the xy edge and of the xz edge.
      const Vector3d xy_i(cos_table[n - i], sin_table[n - i], 0);
      const Vector3d xz_i(cos_table[n - i], 0, sin_table[n - i]);
      octant[t][s] =
        (arc_intersection(xz_k, yz_k, xy_j, yz_j) + arc_intersection(xz_k, yz_k, xy_i, xz_i) +
         arc_intersection(xy_j, yz_j, xy_i, xz_i))
          .normalized();
    }
  }

  auto polyset = std::make_unique<PolySet>(3, /*convex*/ true);
  polyset->vertices.reserve(4 * n * n + 2);
  polyset->indices.reserve(8 * n * n);

  std::vector<int> ring_start(2 * n + 1);
  for (int ring = 0; ring <= 2 * n; ++ring) {
    ring_start[ring] = static_cast<int>(polyset->vertices.size());
    const bool top = ring <= n;
    const int t = top ? ring : 2 * n - ring;  // rings away from the nearest pole
    const int k = n - t;                      // edge points away from the equator
    if (t == 0) {
      polyset->vertices.emplace_back(0, 0, top ? r : -r);
      continue;
    }
    for (int q = 0; q < 4; ++q) {
      for (int s = 0; s < t; ++s) {
        if (t == n) {
          // Equator: the vertices of circle(r) with the same number of fragments.
          const double phi = (360.0 * (q * n + s)) / fragments;
          polyset->vertices.emplace_back(r * cos_degrees(phi), r * sin_degrees(phi), 0);
        } else if (s == 0) {
          // Meridian in the xz plane (even q) or yz plane (odd q): the vertices
          // of that same circle rotated by rotate([90, 0, 0]) or rotate([90, 0, 90]).
          const int idx = top ? (q < 2 ? k : 2 * n - k) : (q < 2 ? 4 * n - k : 2 * n + k);
          const double phi = (360.0 * idx) / fragments;
          const double c = r * cos_degrees(phi);
          const double z = r * sin_degrees(phi);
          if (q % 2 == 0) {
            polyset->vertices.emplace_back(c, 0, z);
          } else {
            polyset->vertices.emplace_back(0, c, z);
          }
        } else {
          const Vector3d& p = octant[t][s];
          const double z = top ? p.z() : -p.z();
          switch (q) {
          case 0:  polyset->vertices.emplace_back(r * p.x(), r * p.y(), r * z); break;
          case 1:  polyset->vertices.emplace_back(-r * p.y(), r * p.x(), r * z); break;
          case 2:  polyset->vertices.emplace_back(-r * p.x(), -r * p.y(), r * z); break;
          default: polyset->vertices.emplace_back(r * p.y(), -r * p.x(), r * z); break;
          }
        }
      }
    }
  }
  assert(polyset->vertices.size() == static_cast<size_t>(4 * n * n + 2));

  // Triangulate the band between ring 'inner' (t vertices per quadrant) and the
  // adjacent ring 'outer' (t + 1 per quadrant). The triangles are wound
  // counterclockwise as seen from outside for the top hemisphere, and
  // 'mirrored' reverses that for the bottom one.
  const auto stitch = [&](int inner, int outer, int t, bool mirrored) {
    const int inner_count = 4 * t;
    const int outer_count = 4 * (t + 1);
    const auto inner_index = [&](int i) {
      return t == 0 ? ring_start[inner] : ring_start[inner] + i % inner_count;
    };
    const auto outer_index = [&](int i) { return ring_start[outer] + i % outer_count; };
    const auto add = [&](int a, int b, int c) {
      if (mirrored) {
        polyset->indices.push_back({a, c, b});
      } else {
        polyset->indices.push_back({a, b, c});
      }
    };
    for (int q = 0; q < 4; ++q) {
      for (int s = 0; s <= t; ++s) {
        const int i = q * t + s;
        const int o = q * (t + 1) + s;
        add(inner_index(i), outer_index(o), outer_index(o + 1));
        if (s < t) {
          add(inner_index(i), outer_index(o + 1), inner_index(i + 1));
        }
      }
    }
  };
  for (int t = 0; t < n; ++t) {
    stitch(t, t + 1, t, false);
    stitch(2 * n - t, 2 * n - t - 1, t, true);
  }
  assert(polyset->indices.size() == static_cast<size_t>(8 * n * n));

  return polyset;
}

// The original sphere: rings of latitude, offset by half a step so there are
// no vertices on the poles, and none on the equator either (only on the x and y
// axes, when num_fragments is 4i+2).
std::unique_ptr<PolySet> SphereNode::createGeometryOrig(int num_fragments) const
{
  auto num_rings = (num_fragments + 1) / 2;
  // Uncomment the following three lines to enable experimental sphere
  // tessellation
  //  if (num_rings % 2 == 0) num_rings++; // To ensure that the middle ring is at
  //  phi == 0 degrees

  auto polyset = std::make_unique<PolySet>(3, /*convex*/ true);
  polyset->vertices.reserve(num_rings * num_fragments);

  // double offset = 0.5 * ((fragments / 2) % 2);
  for (auto i = 0; i < num_rings; ++i) {
    //                double phi = (180.0 * (i + offset)) / (fragments/2);
    const double phi = (180.0 * (i + 0.5)) / num_rings;
    const double radius = r * sin_degrees(phi);
    generate_circle(std::back_inserter(polyset->vertices), radius, r * cos_degrees(phi), num_fragments);
  }

  polyset->indices.push_back({});
  for (int i = 0; i < num_fragments; ++i) {
    polyset->indices.back().push_back(i);
  }

  for (auto i = 0; i < num_rings - 1; ++i) {
    for (auto r = 0; r < num_fragments; ++r) {
      polyset->indices.push_back({
        i * num_fragments + (r + 1) % num_fragments,
        i * num_fragments + r,
        (i + 1) * num_fragments + r,
        (i + 1) * num_fragments + (r + 1) % num_fragments,
      });
    }
  }

  polyset->indices.push_back({});
  for (int i = 0; i < num_fragments; ++i) {
    polyset->indices.back().push_back(num_rings * num_fragments - i - 1);
  }

  return polyset;
}

std::unique_ptr<const Geometry> SphereNode::createGeometry() const
{
  if (this->r <= 0 || !std::isfinite(this->r)) {
    return PolySet::createEmpty();
  }

  const int num_fragments = discretizer.getCircularSegmentCount(r).value_or(3);

  if (style == "octa") {
    return createGeometryOcta(num_fragments);
  } else {
    return createGeometryOrig(num_fragments);
  }
}

static std::shared_ptr<AbstractNode> builtin_sphere(const ModuleInstantiation *inst, Arguments arguments)
{
  Parameters parameters =
    Parameters::parse(std::move(arguments), inst->location(), {"r"}, {"d", "style"});

  auto node = std::make_shared<SphereNode>(inst, CurveDiscretizer(parameters, inst->location()));

  const auto r = parameters.lookupRadius("d", "r");
  if (r) {
    node->r = r->toDouble();
    if (OpenSCAD::rangeCheck && (node->r <= 0 || !std::isfinite(node->r))) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "sphere(r=%1$s)",
          r->toEchoStringNoThrow());
    }
  }

  (void)parameters.valid("style", Value::Type::STRING);
  const std::string style = parameters.get("style", node->style);
  if (style == "orig" || style == "octa") {
    node->style = style;
  } else {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "sphere(style=\"%1$s\") is not \"orig\" or \"octa\", using \"%2$s\"", style, node->style);
  }

  return node;
}

std::string CylinderNode::toString() const
{
  std::ostringstream stream;
  stream << "cylinder(" << discretizer << ", h = " << h << ", r1 = " << r1 << ", r2 = " << r2
         << ", center = " << (center ? "true" : "false") << ")";
  return stream.str();
}

std::unique_ptr<const Geometry> CylinderNode::createGeometry() const
{
  if (this->h <= 0 || !std::isfinite(this->h) || this->r1 < 0 || !std::isfinite(this->r1) ||
      this->r2 < 0 || !std::isfinite(this->r2) || (this->r1 <= 0 && this->r2 <= 0)) {
    return PolySet::createEmpty();
  }

  int num_fragments = discretizer.getCircularSegmentCount(std::fmax(this->r1, this->r2)).value_or(3);

  double z1, z2;
  if (this->center) {
    z1 = -this->h / 2;
    z2 = +this->h / 2;
  } else {
    z1 = 0;
    z2 = this->h;
  }

  bool cone = (r2 == 0.0);
  bool inverted_cone = (r1 == 0.0);

  auto polyset = std::make_unique<PolySet>(3, /*convex*/ true);
  polyset->vertices.reserve((cone || inverted_cone) ? num_fragments + 1 : 2 * num_fragments);

  if (inverted_cone) {
    polyset->vertices.emplace_back(0.0, 0.0, z1);
  } else {
    generate_circle(std::back_inserter(polyset->vertices), r1, z1, num_fragments);
  }
  if (cone) {
    polyset->vertices.emplace_back(0.0, 0.0, z2);
  } else {
    generate_circle(std::back_inserter(polyset->vertices), r2, z2, num_fragments);
  }

  for (int i = 0; i < num_fragments; ++i) {
    int j = (i + 1) % num_fragments;
    if (cone) polyset->indices.push_back({i, j, num_fragments});
    else if (inverted_cone) polyset->indices.push_back({0, j + 1, i + 1});
    else polyset->indices.push_back({i, j, j + num_fragments, i + num_fragments});
  }

  if (!inverted_cone) {
    polyset->indices.push_back({});
    for (int i = 0; i < num_fragments; ++i) {
      polyset->indices.back().push_back(num_fragments - i - 1);
    }
  }
  if (!cone) {
    polyset->indices.push_back({});
    int offset = inverted_cone ? 1 : num_fragments;
    for (int i = 0; i < num_fragments; ++i) {
      polyset->indices.back().push_back(offset + i);
    }
  }

  return polyset;
}

static std::shared_ptr<AbstractNode> builtin_cylinder(const ModuleInstantiation *inst,
                                                      Arguments arguments)
{
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(),
                                            {"h", "r1", "r2", "center"}, {"r", "d", "d1", "d2"});
  auto node = std::make_shared<CylinderNode>(inst, CurveDiscretizer(parameters, inst->location()));

  if (parameters["h"].type() == Value::Type::NUMBER) {
    node->h = parameters["h"].toDouble();
  }

  auto r = parameters.lookupRadius("d", "r");
  auto r1 = parameters.lookupRadius("d1", "r1");
  auto r2 = parameters.lookupRadius("d2", "r2");
  if (r && (r1 || r2)) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "Cylinder parameters ambiguous");
  }

  if (r) {
    node->r1 = r->toDouble();
    node->r2 = r->toDouble();
  }
  if (r1) {
    node->r1 = r1->toDouble();
  }
  if (r2) {
    node->r2 = r2->toDouble();
  }

  if (OpenSCAD::rangeCheck) {
    if (node->h <= 0 || !std::isfinite(node->h)) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "cylinder(h=%1$s, ...)",
          parameters["h"].toEchoStringNoThrow());
    }
    if (node->r1 < 0 || node->r2 < 0 || (node->r1 == 0 && node->r2 == 0) || !std::isfinite(node->r1) ||
        !std::isfinite(node->r2)) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "cylinder(r1=%1$s, r2=%2$s, ...)", (r1 ? r1->toEchoStringNoThrow() : r->toEchoStringNoThrow()),
          (r2 ? r2->toEchoStringNoThrow() : r->toEchoStringNoThrow()));
    }
  }

  if (parameters["center"].type() == Value::Type::BOOL) {
    node->center = parameters["center"].toBool();
  }

  return node;
}

std::string PolyhedronNode::toString() const
{
  std::ostringstream stream;
  stream << "polyhedron(points = [";
  bool firstPoint = true;
  for (const auto& point : this->points) {
    if (firstPoint) {
      firstPoint = false;
    } else {
      stream << ", ";
    }
    stream << "[" << point[0] << ", " << point[1] << ", " << point[2] << "]";
  }
  stream << "], faces = [";
  bool firstFace = true;
  for (const auto& face : this->faces) {
    if (firstFace) {
      firstFace = false;
    } else {
      stream << ", ";
    }
    stream << "[";
    bool firstIndex = true;
    for (const auto& index : face) {
      if (firstIndex) {
        firstIndex = false;
      } else {
        stream << ", ";
      }
      stream << index;
    }
    stream << "]";
  }
  stream << "], convexity = " << this->convexity << ")";
  return stream.str();
}

std::unique_ptr<const Geometry> PolyhedronNode::createGeometry() const
{
  auto p = PolySet::createEmpty();
  p->setConvexity(this->convexity);
  p->vertices = this->points;
  p->indices = this->faces;
  bool is_triangular = true;
  for (auto& poly : p->indices) {
    std::reverse(poly.begin(), poly.end());
    if (is_triangular && poly.size() > 3) {
      is_triangular = false;
    }
  }
  p->setTriangular(is_triangular);
  return p;
}

static std::shared_ptr<AbstractNode> builtin_polyhedron(const ModuleInstantiation *inst,
                                                        Arguments arguments)
{
  auto node = std::make_shared<PolyhedronNode>(inst);

  Parameters parameters =
    Parameters::parse(std::move(arguments), inst->location(), {"points", "faces", "convexity"});

  if (parameters["points"].type() != Value::Type::VECTOR) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "Unable to convert points = %1$s to a vector of coordinates",
        parameters["points"].toEchoStringNoThrow());
    return node;
  }
  node->points.reserve(parameters["points"].toVector().size());
  for (const Value& pointValue : parameters["points"].toVector()) {
    Vector3d point;
    if (!pointValue.getVec3(point[0], point[1], point[2], 0.0) || !std::isfinite(point[0]) ||
        !std::isfinite(point[1]) || !std::isfinite(point[2])) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unable to convert points[%1$d] = %2$s to a vec3 of numbers", node->points.size(),
          pointValue.toEchoStringNoThrow());
      node->points.push_back({0, 0, 0});
    } else {
      node->points.push_back(point);
    }
  }

  const Value *faces = &parameters["faces"];
  if (faces->type() != Value::Type::VECTOR) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "Unable to convert faces = %1$s to a vector of vector of point indices",
        faces->toEchoStringNoThrow());
    return node;
  }
  size_t faceIndex = 0;
  node->faces.reserve(faces->toVector().size());
  for (const Value& faceValue : faces->toVector()) {
    if (faceValue.type() != Value::Type::VECTOR) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unable to convert faces[%1$d] = %2$s to a vector of numbers", faceIndex,
          faceValue.toEchoStringNoThrow());
    } else {
      size_t pointIndexIndex = 0;
      IndexedFace face;
      for (const Value& pointIndexValue : faceValue.toVector()) {
        if (pointIndexValue.type() != Value::Type::NUMBER) {
          LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
              "Unable to convert faces[%1$d][%2$d] = %3$s to a number", faceIndex, pointIndexIndex,
              pointIndexValue.toEchoStringNoThrow());
        } else {
          auto pointIndex = (size_t)pointIndexValue.toDouble();
          if (pointIndex < node->points.size()) {
            face.push_back(pointIndex);
          } else {
            LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
                "Point index %1$d is out of bounds (from faces[%2$d][%3$d])", pointIndex, faceIndex,
                pointIndexIndex);
          }
        }
        pointIndexIndex++;
      }
      // FIXME: Print an error message if < 3 vertices are specified
      if (face.size() >= 3) {
        node->faces.push_back(std::move(face));
      }
    }
    faceIndex++;
  }

  node->convexity = (int)parameters["convexity"].toDouble();
  if (node->convexity < 1) node->convexity = 1;

  return node;
}

std::unique_ptr<const Geometry> SquareNode::createGeometry() const
{
  if (this->x <= 0 || !std::isfinite(this->x) || this->y <= 0 || !std::isfinite(this->y)) {
    return std::make_unique<Polygon2d>();
  }

  Vector2d v1(0, 0);
  Vector2d v2(this->x, this->y);
  if (this->center) {
    v1 -= Vector2d(this->x / 2, this->y / 2);
    v2 -= Vector2d(this->x / 2, this->y / 2);
  }

  Outline2d o;
  o.vertices = {v1, {v2[0], v1[1]}, v2, {v1[0], v2[1]}};
  return std::make_unique<Polygon2d>(o);
}

static std::shared_ptr<AbstractNode> builtin_square(const ModuleInstantiation *inst, Arguments arguments)
{
  auto node = std::make_shared<SquareNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"size", "center"});

  const auto& size = parameters["size"];
  if (size.isDefined()) {
    bool converted = false;
    converted |= size.getDouble(node->x);
    converted |= size.getDouble(node->y);
    converted |= size.getVec2(node->x, node->y);
    if (!converted) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unable to convert square(size=%1$s, ...) parameter to a number or a vec2 of numbers",
          size.toEchoStringNoThrow());
    } else if (OpenSCAD::rangeCheck) {
      bool ok = true;
      ok &= (node->x > 0) && (node->y > 0);
      ok &= std::isfinite(node->x) && std::isfinite(node->y);
      if (!ok) {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
            "square(size=%1$s, ...)", size.toEchoStringNoThrow());
      }
    }
  }
  if (parameters["center"].type() == Value::Type::BOOL) {
    node->center = parameters["center"].toBool();
  }

  return node;
}

std::string CircleNode::toString() const
{
  std::ostringstream stream;
  stream << "circle(" << discretizer << ", r = " << r << ")";
  return stream.str();
}

std::unique_ptr<const Geometry> CircleNode::createGeometry() const
{
  if (this->r <= 0 || !std::isfinite(this->r)) {
    return std::make_unique<Polygon2d>();
  }

  int num_fragments = discretizer.getCircularSegmentCount(this->r).value_or(3);
  Outline2d o;
  o.vertices.resize(num_fragments);
  for (int i = 0; i < num_fragments; ++i) {
    double phi = (360.0 * i) / num_fragments;
    o.vertices[i] = {this->r * cos_degrees(phi), this->r * sin_degrees(phi)};
  }
  return std::make_unique<Polygon2d>(o);
}

static std::shared_ptr<AbstractNode> builtin_circle(const ModuleInstantiation *inst, Arguments arguments)
{
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"r"}, {"d"});
  auto node = std::make_shared<CircleNode>(inst, CurveDiscretizer(parameters, inst->location()));

  const auto r = parameters.lookupRadius("d", "r");
  if (r) {
    node->r = r->toDouble();
    if (OpenSCAD::rangeCheck && ((node->r <= 0) || !std::isfinite(node->r))) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "circle(r=%1$s)",
          r->toEchoStringNoThrow());
    }
  }

  return node;
}

std::string PolygonNode::toString() const
{
  std::ostringstream stream;
  stream << "polygon(points = [";
  bool firstPoint = true;
  for (const auto& point : this->points) {
    if (firstPoint) {
      firstPoint = false;
    } else {
      stream << ", ";
    }
    stream << "[" << point[0] << ", " << point[1] << "]";
  }
  stream << "], paths = ";
  if (this->paths.empty()) {
    stream << "undef";
  } else {
    stream << "[";
    bool firstPath = true;
    for (const auto& path : this->paths) {
      if (firstPath) {
        firstPath = false;
      } else {
        stream << ", ";
      }
      stream << "[";
      bool firstIndex = true;
      for (const auto& index : path) {
        if (firstIndex) {
          firstIndex = false;
        } else {
          stream << ", ";
        }
        stream << index;
      }
      stream << "]";
    }
    stream << "]";
  }
  stream << ", convexity = " << this->convexity << ")";
  return stream.str();
}

std::unique_ptr<const Geometry> PolygonNode::createGeometry() const
{
  auto p = std::make_unique<Polygon2d>();
  if (this->paths.empty() && this->points.size() > 2) {
    Outline2d outline;
    for (const auto& point : this->points) {
      outline.vertices.push_back(point);
    }
    p->addOutline(outline);
  } else {
    bool positive = true;  // First outline is positive
    for (const auto& path : this->paths) {
      Outline2d outline;
      for (const auto& index : path) {
        assert(index < this->points.size());
        const auto& point = points[index];
        outline.vertices.push_back(point);
      }
      outline.positive = positive;
      p->addOutline(outline);
      positive = false;  // Subsequent outlines are holes
    }
  }
  if (p->outlines().size() > 0) {
    p->setConvexity(convexity);
  }
  return p;
}

static std::shared_ptr<AbstractNode> builtin_polygon(const ModuleInstantiation *inst,
                                                     Arguments arguments)
{
  auto node = std::make_shared<PolygonNode>(inst);

  Parameters parameters =
    Parameters::parse(std::move(arguments), inst->location(), {"points", "paths", "convexity"});

  if (parameters["points"].type() != Value::Type::VECTOR) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "Unable to convert points = %1$s to a vector of coordinates",
        parameters["points"].toEchoStringNoThrow());
    return node;
  }
  for (const Value& pointValue : parameters["points"].toVector()) {
    Vector2d point;
    if (!pointValue.getVec2(point[0], point[1]) || !std::isfinite(point[0]) ||
        !std::isfinite(point[1])) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unable to convert points[%1$d] = %2$s to a vec2 of numbers", node->points.size(),
          pointValue.toEchoStringNoThrow());
      node->points.push_back({0, 0});
    } else {
      node->points.push_back(point);
    }
  }

  if (parameters["paths"].type() == Value::Type::VECTOR) {
    size_t pathIndex = 0;
    for (const Value& pathValue : parameters["paths"].toVector()) {
      if (pathValue.type() != Value::Type::VECTOR) {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
            "Unable to convert paths[%1$d] = %2$s to a vector of numbers", pathIndex,
            pathValue.toEchoStringNoThrow());
      } else {
        size_t pointIndexIndex = 0;
        std::vector<size_t> path;
        for (const Value& pointIndexValue : pathValue.toVector()) {
          if (pointIndexValue.type() != Value::Type::NUMBER) {
            LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
                "Unable to convert paths[%1$d][%2$d] = %3$s to a number", pathIndex, pointIndexIndex,
                pointIndexValue.toEchoStringNoThrow());
          } else {
            auto pointIndex = (size_t)pointIndexValue.toDouble();
            if (pointIndex < node->points.size()) {
              path.push_back(pointIndex);
            } else {
              LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
                  "Point index %1$d is out of bounds (from paths[%2$d][%3$d])", pointIndex, pathIndex,
                  pointIndexIndex);
            }
          }
          pointIndexIndex++;
        }
        node->paths.push_back(std::move(path));
      }
      pathIndex++;
    }
  } else if (parameters["paths"].type() != Value::Type::UNDEFINED) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "Unable to convert paths = %1$s to a vector of vector of point indices",
        parameters["paths"].toEchoStringNoThrow());
    return node;
  }

  node->convexity = (int)parameters["convexity"].toDouble();
  if (node->convexity < 1) node->convexity = 1;

  return node;
}

void register_builtin_primitives()
{
  Builtins::init("cube", new BuiltinModule(builtin_cube),
                 {
                   "cube(size)",
                   "cube([width, depth, height])",
                   "cube([width, depth, height], center = true)",
                 });

  Builtins::init("sphere", new BuiltinModule(builtin_sphere),
                 {
                   "sphere(radius)",
                   "sphere(r = radius)",
                   "sphere(d = diameter)",
                   "sphere(r = radius, style = \"octa\")",
                 });

  Builtins::init("cylinder", new BuiltinModule(builtin_cylinder),
                 {
                   "cylinder(h, r1, r2)",
                   "cylinder(h = height, r = radius, center = true)",
                   "cylinder(h = height, r1 = bottom, r2 = top, center = true)",
                   "cylinder(h = height, d = diameter, center = true)",
                   "cylinder(h = height, d1 = bottom, d2 = top, center = true)",
                 });

  Builtins::init("polyhedron", new BuiltinModule(builtin_polyhedron),
                 {
                   "polyhedron(points, faces, convexity)",
                 });

  Builtins::init("square", new BuiltinModule(builtin_square),
                 {
                   "square(size, center = true)",
                   "square([width,height], center = true)",
                 });

  Builtins::init("circle", new BuiltinModule(builtin_circle),
                 {
                   "circle(radius)",
                   "circle(r = radius)",
                   "circle(d = diameter)",
                 });

  Builtins::init("polygon", new BuiltinModule(builtin_polygon),
                 {
                   "polygon([points])",
                   "polygon([points], [paths])",
                 });
}

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
#include "core/Builtins.h"
#include "core/Children.h"
#include "core/ModuleInstantiation.h"
#include "core/Parameters.h"
#include "geometry/PolySet.h"
#include "geometry/Polygon2d.h"
#include "utils/calc.h"
#include "core/node.h"
#include "utils/degree_trig.h"
#include "core/module.h"
#include "utils/printutils.h"
#include <algorithm>
#include <utility>
#include <boost/assign/std/vector.hpp>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace boost::assign; // bring 'operator+=()' into scope

#define F_MINIMUM 0.01

template <class InsertIterator>
static void generate_circle(InsertIterator iter, double r, double z, int fragments) {
  for (int i = 0; i < fragments; ++i) {
    double phi = (360.0 * i) / fragments;
    *(iter++) = {r * cos_degrees(phi), r * sin_degrees(phi), z};
  }
}

/**
 * Return a radius value by looking up both a diameter and radius variable.
 * The diameter has higher priority, so if found an additionally set radius
 * value is ignored.
 *
 * @param parameters parameters with variable values.
 * @param inst containing instantiation.
 * @param radius_var name of the variable to lookup for the radius value.
 * @param diameter_var name of the variable to lookup for the diameter value.
 * @return radius value of type Value::Type::NUMBER or Value::Type::UNDEFINED if both
 *         variables are invalid or not set.
 */
static Value lookup_radius(const Parameters& parameters, const ModuleInstantiation *inst, const std::string& diameter_var, const std::string& radius_var)
{
  const auto& d = parameters[diameter_var];
  const auto& r = parameters[radius_var];
  const auto r_defined = (r.type() == Value::Type::NUMBER);

  if (d.type() == Value::Type::NUMBER) {
    if (r_defined) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Ignoring radius variable '%1$s' as diameter '%2$s' is defined too.", radius_var, diameter_var);
    }
    return d.toDouble() / 2.0;
  } else if (r_defined) {
    return r.clone();
  } else {
    return Value::undefined.clone();
  }
}

static void set_fragments(const Parameters& parameters, const ModuleInstantiation *inst, double& fn, double& fs, double& fa)
{
  fn = parameters["$fn"].toDouble();
  fs = parameters["$fs"].toDouble();
  fa = parameters["$fa"].toDouble();

  if (fs < F_MINIMUM) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "$fs too small - clamping to %1$f", F_MINIMUM);
    fs = F_MINIMUM;
  }
  if (fa < F_MINIMUM) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "$fa too small - clamping to %1$f", F_MINIMUM);
    fa = F_MINIMUM;
  }
}



std::unique_ptr<const Geometry> CubeNode::createGeometry() const
{
  if (this->x <= 0 || !std::isfinite(this->x)
    || this->y <= 0 || !std::isfinite(this->y)
    || this->z <= 0 || !std::isfinite(this->z)
    ) {
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
  int dimension = 3;
  auto ps = std::make_unique<PolySet>(3, /*convex*/true);
  for (int i = 0; i < 8; i++) {
    ps->vertices.emplace_back(i & 1 ? x2 : x1, i & 2 ? y2 : y1,
                              i & 4 ? z2 : z1);
  }
  ps->indices = {
      {4, 5, 7, 6}, // top
      {2, 3, 1, 0}, // bottom
      {0, 1, 5, 4}, // front
      {1, 3, 7, 5}, // right
      {3, 2, 6, 7}, // back
      {2, 0, 4, 6}, // left
  };

  return ps;
}

static std::shared_ptr<AbstractNode> builtin_cube(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<CubeNode>(inst);

  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", node->name());
  }

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"size", "center"});

  const auto& size = parameters["size"];
  if (size.isDefined()) {
    bool converted = false;
    converted |= size.getDouble(node->x);
    converted |= size.getDouble(node->y);
    converted |= size.getDouble(node->z);
    converted |= size.getVec3(node->x, node->y, node->z);
    if (!converted) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Unable to convert cube(size=%1$s, ...) parameter to a number or a vec3 of numbers", size.toEchoStringNoThrow());
    } else if (OpenSCAD::rangeCheck) {
      bool ok = (node->x > 0) && (node->y > 0) && (node->z > 0);
      ok &= std::isfinite(node->x) && std::isfinite(node->y) && std::isfinite(node->z);
      if (!ok) {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "cube(size=%1$s, ...)", size.toEchoStringNoThrow());
      }
    }
  }
  if (parameters["center"].type() == Value::Type::BOOL) {
    node->center = parameters["center"].toBool();
  }

  return node;
}

std::unique_ptr<const Geometry> SphereNode::createGeometry() const
{
  if (this->r <= 0 || !std::isfinite(this->r)) {
    return PolySet::createEmpty();
  }

  auto num_fragments = Calc::get_fragments_from_r(r, fn, fs, fa);
  size_t num_rings = (num_fragments + 1) / 2;
  // Uncomment the following three lines to enable experimental sphere
  // tessellation
  //  if (num_rings % 2 == 0) num_rings++; // To ensure that the middle ring is at
  //  phi == 0 degrees

  auto polyset = std::make_unique<PolySet>(3, /*convex*/true);
  polyset->vertices.reserve(num_rings * num_fragments);

  // double offset = 0.5 * ((fragments / 2) % 2);
  for (int i = 0; i < num_rings; ++i) {
    //                double phi = (180.0 * (i + offset)) / (fragments/2);
    const double phi = (180.0 * (i + 0.5)) / num_rings;
    const double radius = r * sin_degrees(phi);
    generate_circle(std::back_inserter(polyset->vertices), radius, r * cos_degrees(phi), num_fragments);
  }

  polyset->indices.push_back({});
  for (int i = 0; i < num_fragments; ++i) {
    polyset->indices.back().push_back(i);
  }

  for (int i = 0; i < num_rings - 1; ++i) {
    for (int r=0;r<num_fragments;++r) {
      polyset->indices.push_back({
        i*num_fragments+(r+1)%num_fragments,
        i*num_fragments+r,
        (i+1)*num_fragments+r,
        (i+1)*num_fragments+(r+1)%num_fragments,
      });
    }
  }

  polyset->indices.push_back({});
  for (int i = 0; i < num_fragments; ++i) {
    polyset->indices.back().push_back(num_rings * num_fragments - i - 1);
  }

  return polyset;
}

static std::shared_ptr<AbstractNode> builtin_sphere(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<SphereNode>(inst);

  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", node->name());
  }

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"r"}, {"d"});

  set_fragments(parameters, inst, node->fn, node->fs, node->fa);
  const auto r = lookup_radius(parameters, inst, "d", "r");
  if (r.type() == Value::Type::NUMBER) {
    node->r = r.toDouble();
    if (OpenSCAD::rangeCheck && (node->r <= 0 || !std::isfinite(node->r))) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "sphere(r=%1$s)", r.toEchoStringNoThrow());
    }
  }

  return node;
}



std::unique_ptr<const Geometry> CylinderNode::createGeometry() const
{
  if (
    this->h <= 0 || !std::isfinite(this->h)
    || this->r1 < 0 || !std::isfinite(this->r1)
    || this->r2 < 0 || !std::isfinite(this->r2)
    || (this->r1 <= 0 && this->r2 <= 0)
    ) {
    return PolySet::createEmpty();
  }

  auto num_fragments = Calc::get_fragments_from_r(std::fmax(this->r1, this->r2), this->fn, this->fs, this->fa);

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

  auto polyset = std::make_unique<PolySet>(3, /*convex*/true);
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
    else if (inverted_cone) polyset->indices.push_back({0, j+1, i+1});
    else polyset->indices.push_back({i, j, j+num_fragments, i+num_fragments});
  }

  if (!inverted_cone) {
    polyset->indices.push_back({});
    for (int i = 0; i < num_fragments; ++i) {
      polyset->indices.back().push_back(num_fragments-i-1);
    }
  }
  if (!cone) {
    polyset->indices.push_back({});
    int offset = inverted_cone ? 1 : num_fragments;
    for (int i = 0; i < num_fragments; ++i) {
      polyset->indices.back().push_back(offset+i);
    }
  }

  return polyset;
}

static std::shared_ptr<AbstractNode> builtin_cylinder(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<CylinderNode>(inst);

  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", node->name());
  }

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"h", "r1", "r2", "center"}, {"r", "d", "d1", "d2"});

  set_fragments(parameters, inst, node->fn, node->fs, node->fa);
  if (parameters["h"].type() == Value::Type::NUMBER) {
    node->h = parameters["h"].toDouble();
  }

  auto r = lookup_radius(parameters, inst, "d", "r");
  auto r1 = lookup_radius(parameters, inst, "d1", "r1");
  auto r2 = lookup_radius(parameters, inst, "d2", "r2");
  if (r.type() == Value::Type::NUMBER &&
      (r1.type() == Value::Type::NUMBER || r2.type() == Value::Type::NUMBER)
      ) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Cylinder parameters ambiguous");
  }

  if (r.type() == Value::Type::NUMBER) {
    node->r1 = r.toDouble();
    node->r2 = r.toDouble();
  }
  if (r1.type() == Value::Type::NUMBER) {
    node->r1 = r1.toDouble();
  }
  if (r2.type() == Value::Type::NUMBER) {
    node->r2 = r2.toDouble();
  }

  if (OpenSCAD::rangeCheck) {
    if (node->h <= 0 || !std::isfinite(node->h)) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "cylinder(h=%1$s, ...)", parameters["h"].toEchoStringNoThrow());
    }
    if (node->r1 < 0 || node->r2 < 0 || (node->r1 == 0 && node->r2 == 0) || !std::isfinite(node->r1) || !std::isfinite(node->r2)) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "cylinder(r1=%1$s, r2=%2$s, ...)",
          (r1.type() == Value::Type::NUMBER ? r1.toEchoStringNoThrow() : r.toEchoStringNoThrow()),
          (r2.type() == Value::Type::NUMBER ? r2.toEchoStringNoThrow() : r.toEchoStringNoThrow()));
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
  p->vertices=this->points;
  p->indices=this->faces;
  bool is_triangular = true;
  for (auto &poly : p->indices) {
    std::reverse(poly.begin(),poly.end());
    if (is_triangular && poly.size() > 3) {
      is_triangular = false;
    }
  }
  p->setTriangular(is_triangular);
  return p;
}

static std::shared_ptr<AbstractNode> builtin_polyhedron(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<PolyhedronNode>(inst);

  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", node->name());
  }

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"points", "faces", "convexity"}, {"triangles"});

  if (parameters["points"].type() != Value::Type::VECTOR) {
    LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert points = %1$s to a vector of coordinates", parameters["points"].toEchoStringNoThrow());
    return node;
  }
  node->points.reserve(parameters["points"].toVector().size());
  for (const Value& pointValue : parameters["points"].toVector()) {
    Vector3d point;
    if (!pointValue.getVec3(point[0], point[1], point[2], 0.0) ||
        !std::isfinite(point[0]) || !std::isfinite(point[1]) || !std::isfinite(point[2])
        ) {
      LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert points[%1$d] = %2$s to a vec3 of numbers", node->points.size(), pointValue.toEchoStringNoThrow());
      node->points.push_back({0, 0, 0});
    } else {
      node->points.push_back(point);
    }
  }

  const Value *faces = nullptr;
  if (parameters["faces"].type() == Value::Type::UNDEFINED && parameters["triangles"].type() != Value::Type::UNDEFINED) {
    // backwards compatible
    LOG(message_group::Deprecated, inst->location(), parameters.documentRoot(), "polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
    faces = &parameters["triangles"];
  } else {
    faces = &parameters["faces"];
  }
  if (faces->type() != Value::Type::VECTOR) {
    LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert faces = %1$s to a vector of vector of point indices", faces->toEchoStringNoThrow());
    return node;
  }
  size_t faceIndex = 0;
  node->faces.reserve(faces->toVector().size());
  for (const Value& faceValue : faces->toVector()) {
    if (faceValue.type() != Value::Type::VECTOR) {
      LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert faces[%1$d] = %2$s to a vector of numbers", faceIndex, faceValue.toEchoStringNoThrow());
    } else {
      size_t pointIndexIndex = 0;
      IndexedFace face;
      for (const Value& pointIndexValue : faceValue.toVector()) {
        if (pointIndexValue.type() != Value::Type::NUMBER) {
          LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert faces[%1$d][%2$d] = %3$s to a number", faceIndex, pointIndexIndex, pointIndexValue.toEchoStringNoThrow());
        } else {
          auto pointIndex = (size_t)pointIndexValue.toDouble();
          if (pointIndex < node->points.size()) {
            face.push_back(pointIndex);
          } else {
            LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Point index %1$d is out of bounds (from faces[%2$d][%3$d])", pointIndex, faceIndex, pointIndexIndex);
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
  if (this->x <= 0 || !std::isfinite(this->x) ||
      this->y <= 0 || !std::isfinite(this->y)) {
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

static std::shared_ptr<AbstractNode> builtin_square(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<SquareNode>(inst);

  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", node->name());
  }

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"size", "center"});

  const auto& size = parameters["size"];
  if (size.isDefined()) {
    bool converted = false;
    converted |= size.getDouble(node->x);
    converted |= size.getDouble(node->y);
    converted |= size.getVec2(node->x, node->y);
    if (!converted) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Unable to convert square(size=%1$s, ...) parameter to a number or a vec2 of numbers", size.toEchoStringNoThrow());
    } else if (OpenSCAD::rangeCheck) {
      bool ok = true;
      ok &= (node->x > 0) && (node->y > 0);
      ok &= std::isfinite(node->x) && std::isfinite(node->y);
      if (!ok) {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "square(size=%1$s, ...)", size.toEchoStringNoThrow());
      }
    }
  }
  if (parameters["center"].type() == Value::Type::BOOL) {
    node->center = parameters["center"].toBool();
  }

  return node;
}

std::unique_ptr<const Geometry> CircleNode::createGeometry() const
{
  if (this->r <= 0 || !std::isfinite(this->r)) {
    return std::make_unique<Polygon2d>();
  }

  auto fragments = Calc::get_fragments_from_r(this->r, this->fn, this->fs, this->fa);
  Outline2d o;
  o.vertices.resize(fragments);
  for (int i = 0; i < fragments; ++i) {
    double phi = (360.0 * i) / fragments;
    o.vertices[i] = {this->r * cos_degrees(phi), this->r * sin_degrees(phi)};
  }
  return std::make_unique<Polygon2d>(o);
}

static std::shared_ptr<AbstractNode> builtin_circle(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<CircleNode>(inst);

  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", node->name());
  }

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"r"}, {"d"});

  set_fragments(parameters, inst, node->fn, node->fs, node->fa);
  const auto r = lookup_radius(parameters, inst, "d", "r");
  if (r.type() == Value::Type::NUMBER) {
    node->r = r.toDouble();
    if (OpenSCAD::rangeCheck && ((node->r <= 0) || !std::isfinite(node->r))) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "circle(r=%1$s)", r.toEchoStringNoThrow());
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
    for (const auto& path : this->paths) {
      Outline2d outline;
      for (const auto& index : path) {
        assert(index < this->points.size());
        const auto& point = points[index];
        outline.vertices.push_back(point);
      }
      p->addOutline(outline);
    }
  }
  if (p->outlines().size() > 0) {
    p->setConvexity(convexity);
  }
  return p;
}

static std::shared_ptr<AbstractNode> builtin_polygon(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<PolygonNode>(inst);

  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", node->name());
  }

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"points", "paths", "convexity"});

  if (parameters["points"].type() != Value::Type::VECTOR) {
    LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert points = %1$s to a vector of coordinates", parameters["points"].toEchoStringNoThrow());
    return node;
  }
  for (const Value& pointValue : parameters["points"].toVector()) {
    Vector2d point;
    if (!pointValue.getVec2(point[0], point[1]) ||
        !std::isfinite(point[0]) || !std::isfinite(point[1])
        ) {
      LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert points[%1$d] = %2$s to a vec2 of numbers", node->points.size(), pointValue.toEchoStringNoThrow());
      node->points.push_back({0, 0});
    } else {
      node->points.push_back(point);
    }
  }

  if (parameters["paths"].type() == Value::Type::VECTOR) {
    size_t pathIndex = 0;
    for (const Value& pathValue : parameters["paths"].toVector()) {
      if (pathValue.type() != Value::Type::VECTOR) {
        LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert paths[%1$d] = %2$s to a vector of numbers", pathIndex, pathValue.toEchoStringNoThrow());
      } else {
        size_t pointIndexIndex = 0;
        std::vector<size_t> path;
        for (const Value& pointIndexValue : pathValue.toVector()) {
          if (pointIndexValue.type() != Value::Type::NUMBER) {
            LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert paths[%1$d][%2$d] = %3$s to a number", pathIndex, pointIndexIndex, pointIndexValue.toEchoStringNoThrow());
          } else {
            auto pointIndex = (size_t)pointIndexValue.toDouble();
            if (pointIndex < node->points.size()) {
              path.push_back(pointIndex);
            } else {
              LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Point index %1$d is out of bounds (from paths[%2$d][%3$d])", pointIndex, pathIndex, pointIndexIndex);
            }
          }
          pointIndexIndex++;
        }
        node->paths.push_back(std::move(path));
      }
      pathIndex++;
    }
  } else if (parameters["paths"].type() != Value::Type::UNDEFINED) {
    LOG(message_group::Error, inst->location(), parameters.documentRoot(), "Unable to convert paths = %1$s to a vector of vector of point indices", parameters["paths"].toEchoStringNoThrow());
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

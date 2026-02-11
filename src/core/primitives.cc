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
static void generate_circle(InsertIterator iter, double r, double z, double angle, int fragments)
{
  int fragments_div = fragments;
  if (angle < 360) {
    *(iter++) = {0, 0, z};
    fragments--;
    fragments_div -= 2;
  }
  for (int i = 0; i < fragments; ++i) {
    double phi = (angle * i) / fragments_div;
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
static Value lookup_radius(const Parameters& parameters, const ModuleInstantiation *inst,
                           const std::string& diameter_var, const std::string& radius_var)
{
  const auto& d = parameters[diameter_var];
  const auto& r = parameters[radius_var];
  const auto r_defined = (r.type() == Value::Type::NUMBER);

  if (d.type() == Value::Type::NUMBER) {
    if (r_defined) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Ignoring radius variable %1$s as diameter %2$s is defined too.", quoteVar(radius_var),
          quoteVar(diameter_var));
    }
    return d.toDouble() / 2.0;
  } else if (r_defined) {
    return r.clone();
  } else {
    return Value::undefined.clone();
  }
}

std::unique_ptr<const Geometry> CubeNode::createGeometry() const
{
  if (this->dim[0] <= 0 || !std::isfinite(this->dim[0]) || this->dim[1] <= 0 ||
      !std::isfinite(this->dim[1]) || this->dim[2] <= 0 || !std::isfinite(this->dim[2])) {
    return PolySet::createEmpty();
  }

  double coord1[3], coord2[3], size;
  for (int i = 0; i < 3; i++) {
    switch (i) {
    case 0: size = this->dim[0]; break;
    case 1: size = this->dim[1]; break;
    case 2: size = this->dim[2]; break;
    }
    if (this->center[i] > 0) {
      coord1[i] = 0;
      coord2[i] = size;
    } else if (this->center[i] < 0) {
      coord1[i] = -size;
      coord2[i] = 0;
    } else {
      coord1[i] = -size / 2;
      coord2[i] = size / 2;
    }
  }
  auto ps = std::make_unique<PolySet>(3, /*convex*/ true);
  for (int i = 0; i < 8; i++) {
    ps->vertices.emplace_back(i & 1 ? coord2[0] : coord1[0], i & 2 ? coord2[1] : coord1[1],
                              i & 4 ? coord2[2] : coord1[2]);
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

std::shared_ptr<const Geometry> CubeNode::dragPoint(const Vector3d& pt, const Vector3d& newpt,
                                                    DragResult& result)
{
  if (dim_[0] == 0) {
    dim_[0] = dim[0];
    dim_[1] = dim[1];
    dim_[2] = dim[2];
  }
  if (dragflags) {
    result.modname = "cube";
    result.mods.clear();
  }
  for (int i = 0; i < 3; i++) {
    if (dragflags & (1 << i)) {
      if (center[i] == 1 && fabs(pt[i] - dim_[i]) < 1e-3) {
        this->dim[i] = newpt[i];
        DragMod mod;
        mod.index = 0;
        mod.name = "size";
        mod.arrinfo.push_back(i);
        mod.value = newpt[i];
        result.mods.push_back(mod);
      }
    }
    if (dragflags & (1 << i) && pt[i] != 0) result.anchor[i] = newpt[i];
    else result.anchor[i] = pt[i];
  }
  return std::shared_ptr<const Geometry>(std::move(createGeometry()));
}

static std::shared_ptr<AbstractNode> builtin_cube(const ModuleInstantiation *inst, Arguments arguments)
{
  auto node = std::make_shared<CubeNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"size", "center"});

  const auto& size = parameters["size"];
  if (size.isDefined()) {
    bool converted = false;
    converted |= size.getDouble(node->dim[0]);
    converted |= size.getDouble(node->dim[1]);
    converted |= size.getDouble(node->dim[2]);
    converted |= size.getVec3(node->dim[0], node->dim[1], node->dim[2]);
    if (!converted) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unable to convert cube(size=%1$s, ...) parameter to a number or a vec3 of numbers",
          size.toEchoStringNoThrow());
    } else if (OpenSCAD::rangeCheck) {
      bool ok = (node->dim[0] > 0) && (node->dim[1] > 0) && (node->dim[2] > 0);
      ok &= std::isfinite(node->dim[0]) && std::isfinite(node->dim[1]) && std::isfinite(node->dim[2]);
      if (!ok) {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "cube(size=%1$s, ...)",
            size.toEchoStringNoThrow());
      }
    }
  }
  if (parameters["center"].type() == Value::Type::BOOL) {
    bool cent = parameters["center"].toBool();
    for (int i = 0; i < 3; i++) node->center[i] = cent ? 0 : 1;
  }

  return node;
}

std::string SphereNode::toString() const
{
  std::ostringstream stream;
  stream << "sphere(" << discretizer;
#ifdef ENABLE_PYTHON
  if (r_func != nullptr) stream << ", r_func = " << rand();
  else
#endif
    stream << ", r = " << r;
  stream << ")";
  return stream.str();
}

std::unique_ptr<const Geometry> SphereNode::createGeometry() const
{
  if (this->r <= 0 || !std::isfinite(this->r)) {
    return PolySet::createEmpty();
  }

  int num_fragments = discretizer.getCircularSegmentCount(r, 360.0).value_or(3);
#ifdef ENABLE_PYTHON
  if (this->r_func != nullptr) {
    double fs = discretizer.getMinimalEdgeLength();
    return sphereCreateFuncGeometry(this->r_func, fs, num_fragments);
  }
#endif
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
    generate_circle(std::back_inserter(polyset->vertices), radius, r * cos_degrees(phi), 360.0,
                    num_fragments);
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

std::shared_ptr<const Geometry> SphereNode::dragPoint(const Vector3d& pt, const Vector3d& newpt,
                                                      DragResult& result)
{
  if (dragflags & 1) {
    r = newpt.norm();
  }
  return std::shared_ptr<const Geometry>(std::move(createGeometry()));
}

static std::shared_ptr<AbstractNode> builtin_sphere(const ModuleInstantiation *inst, Arguments arguments)
{
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"r"}, {"d"});

  auto node = std::make_shared<SphereNode>(inst, CurveDiscretizer(parameters, inst->location()));

  const auto r = lookup_radius(parameters, inst, "d", "r");
  if (r.type() == Value::Type::NUMBER) {
    node->r = r.toDouble();
    if (OpenSCAD::rangeCheck && (node->r <= 0 || !std::isfinite(node->r))) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "sphere(r=%1$s)",
          r.toEchoStringNoThrow());
    }
  }

  return node;
}

std::string CylinderNode::toString() const
{
  std::ostringstream stream;
  stream << "cylinder(" << discretizer << ", h = " << h << ", r1 = " << r1 << ", r2 = " << r2;
  if (angle != 360) stream << ", angle = " << angle;
  stream << ", center = " << (center ? "true" : "false") << ")";
  return stream.str();
}

std::unique_ptr<const Geometry> CylinderNode::createGeometry() const
{
  if (this->h <= 0 || !std::isfinite(this->h) || this->r1 < 0 || !std::isfinite(this->r1) ||
      this->r2 < 0 || !std::isfinite(this->r2) || (this->r1 <= 0 && this->r2 <= 0) ||
      (this->angle <= 0 || this->angle > 360)) {
    return PolySet::createEmpty();
  }

  int num_fragments =
    discretizer.getCircularSegmentCount(std::fmax(this->r1, this->r2), 360.0).value_or(3);

  if (this->angle < 360) num_fragments++;
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
    generate_circle(std::back_inserter(polyset->vertices), r1, z1, this->angle, num_fragments);
  }
  if (cone) {
    polyset->vertices.emplace_back(0.0, 0.0, z2);
  } else {
    generate_circle(std::back_inserter(polyset->vertices), r2, z2, this->angle, num_fragments);
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

std::shared_ptr<const Geometry> CylinderNode::dragPoint(const Vector3d& pt, const Vector3d& newpt,
                                                        DragResult& result)
{
  Vector3d x_ = newpt;
  if (dragflags & 2) {
    h = x_[2];
  }
  x_[2] = 0;
  if (dragflags & 1) {
    r1 = x_.norm();
    r2 = x_.norm();
  }
  return std::shared_ptr<const Geometry>(std::move(createGeometry()));
}

static std::shared_ptr<AbstractNode> builtin_cylinder(const ModuleInstantiation *inst,
                                                      Arguments arguments)
{
  Parameters parameters =
    Parameters::parse(std::move(arguments), inst->location(), {"h", "r1", "r2", "center", "angle"},
                      {"r", "d", "d1", "d2"});

  auto node = std::make_shared<CylinderNode>(inst, CurveDiscretizer(parameters, inst->location()));
  if (parameters["h"].type() == Value::Type::NUMBER) {
    node->h = parameters["h"].toDouble();
  }

  auto r = lookup_radius(parameters, inst, "d", "r");
  auto r1 = lookup_radius(parameters, inst, "d1", "r1");
  auto r2 = lookup_radius(parameters, inst, "d2", "r2");
  if (r.type() == Value::Type::NUMBER &&
      (r1.type() == Value::Type::NUMBER || r2.type() == Value::Type::NUMBER)) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "Cylinder parameters ambiguous");
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
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "cylinder(h=%1$s, ...)",
          parameters["h"].toEchoStringNoThrow());
    }
    if (node->r1 < 0 || node->r2 < 0 || (node->r1 == 0 && node->r2 == 0) || !std::isfinite(node->r1) ||
        !std::isfinite(node->r2)) {
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
  if (this->colors.size() > 0) {
    stream << "], colors = [";
    bool firstColor = true;
    for (const auto& color : this->colors) {
      if (firstColor) {
        firstColor = false;
      } else {
        stream << ", ";
      }
      stream << "[" << color.r() << ", " << color.g() << ", " << color.b() << "]";
    }
  }

  if (this->color_indices.size() > 0) {
    stream << "], color_indices = [";
    bool firstColInd = true;
    for (const auto& colind : this->color_indices) {
      if (firstColInd) {
        firstColInd = false;
      } else {
        stream << ", ";
      }

      stream << colind;
    }
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
  p->colors = this->colors;
  p->color_indices = this->color_indices;
  return p;
}

static std::shared_ptr<AbstractNode> builtin_polyhedron(const ModuleInstantiation *inst,
                                                        Arguments arguments)
{
  auto node = std::make_shared<PolyhedronNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(),
                                            {"points", "faces", "convexity"}, {"triangles"});

  if (parameters["points"].type() != Value::Type::VECTOR) {
    LOG(message_group::Error, inst->location(), parameters.documentRoot(),
        "Unable to convert points = %1$s to a vector of coordinates",
        parameters["points"].toEchoStringNoThrow());
    return node;
  }
  node->points.reserve(parameters["points"].toVector().size());
  for (const Value& pointValue : parameters["points"].toVector()) {
    Vector3d point;
    if (!pointValue.getVec3(point[0], point[1], point[2], 0.0) || !std::isfinite(point[0]) ||
        !std::isfinite(point[1]) || !std::isfinite(point[2])) {
      LOG(message_group::Error, inst->location(), parameters.documentRoot(),
          "Unable to convert points[%1$d] = %2$s to a vec3 of numbers", node->points.size(),
          pointValue.toEchoStringNoThrow());
      node->points.push_back({0, 0, 0});
    } else {
      node->points.push_back(point);
    }
  }

  const Value *faces = nullptr;
  if (parameters["faces"].type() == Value::Type::UNDEFINED &&
      parameters["triangles"].type() != Value::Type::UNDEFINED) {
    // backwards compatible
    LOG(
      message_group::Deprecated, inst->location(), parameters.documentRoot(),
      "polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
    faces = &parameters["triangles"];
  } else {
    faces = &parameters["faces"];
  }
  if (faces->type() != Value::Type::VECTOR) {
    LOG(message_group::Error, inst->location(), parameters.documentRoot(),
        "Unable to convert faces = %1$s to a vector of vector of point indices",
        faces->toEchoStringNoThrow());
    return node;
  }
  size_t faceIndex = 0;
  node->faces.reserve(faces->toVector().size());
  for (const Value& faceValue : faces->toVector()) {
    if (faceValue.type() != Value::Type::VECTOR) {
      LOG(message_group::Error, inst->location(), parameters.documentRoot(),
          "Unable to convert faces[%1$d] = %2$s to a vector of numbers", faceIndex,
          faceValue.toEchoStringNoThrow());
    } else {
      size_t pointIndexIndex = 0;
      IndexedFace face;
      for (const Value& pointIndexValue : faceValue.toVector()) {
        if (pointIndexValue.type() != Value::Type::NUMBER) {
          LOG(message_group::Error, inst->location(), parameters.documentRoot(),
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

std::unique_ptr<const Geometry> EdgeNode::createGeometry() const
{
  if (this->size <= 0) return std::make_unique<Polygon2d>();

  double beg = 0;
  double end = size;
  if (center) {
    beg = -size / 2;
    end = size / 2;
  } else {
    beg = 0;
    end = size;
  }

  Edge1d e(beg, end);
  e.color = *OpenSCAD::parse_color("#f9d72c");
  Barcode1d b(e);
  return std::make_unique<Barcode1d>(b);
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
  o.color = *OpenSCAD::parse_color("#f9d72c");
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
  stream << "circle(" << discretizer << ", r = " << r;
  if (angle != 360) stream << ", angle = " << angle;
  stream << ")";
  return stream.str();
}

std::unique_ptr<const Geometry> CircleNode::createGeometry() const
{
  if (this->r <= 0 || !std::isfinite(this->r) || angle <= 0 || angle > 360.0) {
    return std::make_unique<Polygon2d>();
  }

  int fragments = discretizer.getCircularSegmentCount(this->r, this->angle).value_or(3);
  Outline2d o;
  int fragments_div = fragments;
  if (this->angle < 360.0) {
    o.vertices.resize(fragments + 1);
    o.vertices[fragments] = {0, 0};
    fragments_div--;
  } else o.vertices.resize(fragments);
  for (int i = 0; i < fragments; ++i) {
    double phi = (this->angle * i) / fragments_div;
    o.vertices[i] = {this->r * cos_degrees(phi), this->r * sin_degrees(phi)};
  }
  o.color = *OpenSCAD::parse_color("#f9d72c");
  return std::make_unique<Polygon2d>(o);
}

static std::shared_ptr<AbstractNode> builtin_circle(const ModuleInstantiation *inst, Arguments arguments)
{
  Parameters parameters =
    Parameters::parse(std::move(arguments), inst->location(), {"r"}, {"d", "angle"});
  auto node = std::make_shared<CircleNode>(inst, CurveDiscretizer(parameters, inst->location()));

  const auto r = lookup_radius(parameters, inst, "d", "r");
  if (r.type() == Value::Type::NUMBER) {
    node->r = r.toDouble();
    if (OpenSCAD::rangeCheck && ((node->r <= 0) || !std::isfinite(node->r))) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "circle(r=%1$s)",
          r.toEchoStringNoThrow());
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
    stream << "[" << point[0] << ", " << point[1];
    if (point[2] != 0) stream << ", " << point[2];
    stream << "]";
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

VectorOfVector2d PolygonNode::createGeometry_sub(const std::vector<Vector3d>& points,
                                                 const std::vector<size_t>& path) const
{
  std::vector<Vector2d> result;
  int n = path.size();
  for (int i = 0; i < n; i++) {
    const auto& ptprev = points[path[(i + n - 1) % n]];
    const auto& ptcur = points[path[i]];
    const auto& ptnext = points[path[(i + 1) % n]];
    if (ptcur[2] == 0) result.push_back(ptcur.head<2>());  // normal point without radius
    else {
      double r = ptcur[2];  // do the corner calculations
      Vector3d dir1 = ptcur - ptprev;
      dir1[2] = 0;
      Vector3d dir2 = ptnext - ptcur;
      dir2[2] = 0;
      Vector3d cr = dir1.cross(dir2);
      Vector3d n1 = Vector3d(-dir1[1], dir1[0], 0).normalized();
      Vector3d n2 = Vector3d(-dir2[1], dir2[0], 0).normalized();
      if (cr[2] < 0) {
        n1 = -n1;
        n2 = -n2;
      }  // concave corner
      Vector3d res;
      linsystem(dir1, cr, dir2, ptnext + n2 * r - ptprev - n1 * r, res, nullptr);
      Vector3d st = ptprev + dir1 * res[0];
      st[2] = 0;
      Vector3d cent = st + r * n1;
      Vector3d en = cent - r * n2;
      double ang_st = atan2(-n1[1], -n1[0]);
      double ang_en = atan2(-n2[1], -n2[0]);
      result.push_back(st.head<2>());
      if (ang_en - ang_st > M_PI) ang_en -= 2 * M_PI;
      if (ang_st - ang_en > M_PI) ang_st -= 2 * M_PI;

      int segs = this->discretizer.getCircularSegmentCount(r, ang_en - ang_st).value_or(3);
      for (int j = 0; j < segs; j++) {
        double ang = ang_st + (ang_en - ang_st) * (j + 1) / (segs + 1);
        result.push_back(Vector2d(cent[0] + r * cos(ang), cent[1] + r * sin(ang)));
      }
      result.push_back(en.head<2>());
    }
  }  // for
  return result;
}
extern bool pythonMainModuleInitialized;
std::unique_ptr<const Geometry> PolygonNode::createGeometry() const
{
  auto p = std::make_unique<Polygon2d>();
  if (this->paths.empty() && this->points.size() > 2) {
    Outline2d outline;
    std::vector<size_t> path;
    for (size_t i = 0; i < this->points.size(); i++) path.push_back(i);
    outline.vertices = createGeometry_sub(this->points, path);
    p->addOutline(outline);
  } else {
    bool positive = true;  // First outline is positive
    for (const auto& path : this->paths) {
      Outline2d outline;
      outline.positive = positive;
      outline.vertices = createGeometry_sub(this->points, path);
      p->addOutline(outline);
      positive = false;  // Subsequent outlines are holes
    }
  }
  if (p->outlines().size() > 0) {
    p->setConvexity(convexity);
  }
  p->setColor(*OpenSCAD::parse_color("#f9d72c"));
  return p;
}

std::string SplineNode::toString() const
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
  stream << "], fn = " << this->fn << "fa = " << this->fa << ", fs = " << this->fs << ")";
  return stream.str();
}

std::vector<Vector2d> SplineNode::draw_arc(int fn, const Vector2d& tang1, double l1,
                                           const Vector2d& tang2, double l2,
                                           const Vector2d& cornerpt) const
{
  std::vector<Vector2d> result;
  if (fn == 1) result.push_back(cornerpt);
  else {
    // estimate ellipsis circumfence
    double l = (l1 - l2) / (l1 + l2);
    double circ = (l1 + l2) * M_PI * (1 + (3 * l * l) / (10 + sqrt(4 - 3 * l * l)));
    Vector2d vx = -tang1 * (l1 - circ / (8 * fn));
    Vector2d vy = tang2 * (l2 - circ / (8 * fn));
    //        printf("i=%d vx %g/%g vy %g/%g\n",i,vx[0], vx[1], vy[0], vy[1]);
    for (int j = 0; j <= fn - 1; j++) {
      double ang = M_PI / 2.0 * j / (fn - 1);
      Vector2d pt = cornerpt + vx * (1 - sin(ang)) + vy * (1 - cos(ang));
      result.push_back(pt);
    }
  }
  return result;
}
std::unique_ptr<const Geometry> SplineNode::createGeometry() const
{
  auto p = std::make_unique<Polygon2d>();
  Outline2d result;
  Vector3d dirz(0, 0, 1);
  Vector3d res;

  std::vector<Vector3d> tangent;
  int n = this->points.size();
  for (int i = 0; i < n; i++) {
    Vector2d dir = (this->points[(i + 1) % n] - this->points[(i + n - 1) % n]).normalized();
    tangent.push_back(Vector3d(dir[0], dir[1], 0));
  }
  for (int i = 0; i < n; i++) {
    // point at corner
    int fn = 1;
    if (this->fn > 0 && this->fn > fn) fn = this->fn;
    if (this->fa > 0 && 90.0 / this->fa > fn) fn = 90.0 / this->fa;

    bool convex = true;
    Vector2d diff = this->points[(i + 1) % n] - this->points[i];
    if (linsystem(tangent[i], tangent[(i + 1) % n], dirz, Vector3d(diff[0], diff[1], 0), res, nullptr))
      convex = false;
    if (res[0] < -1e-6 || res[1] < -1e-6) convex = false;
    if (convex) {
      Vector2d cornerpt = this->points[i] + res[0] * tangent[i].head<2>();
      auto pts =
        draw_arc(fn, tangent[i].head<2>(), res[0], tangent[(i + 1) % n].head<2>(), res[1], cornerpt);
      for (const Vector2d& pt : pts) result.vertices.push_back(pt);

    } else {
      // create midpoint
      Vector2d midpt = (this->points[i] + this->points[(i + 1) % n]) / 2.0;
      Vector2d dir = (this->points[(i + 1) % n] - this->points[i]).normalized();

      // Mid tangent
      Vector2d midtang = (tangent[i] + tangent[(i + 1) % n]).normalized().head<2>().normalized();

      // flip mid tangent
      double l = midtang.dot(dir);
      midtang = 2 * dir * l - midtang;

      Vector2d diff = midpt - this->points[i];

      // 1st arc
      if (linsystem(tangent[i], Vector3d(midtang[0], midtang[1], 0), dirz, Vector3d(diff[0], diff[1], 0),
                    res, nullptr))
        printf("prog errr\n");
      Vector2d cornerpt1 = this->points[i] + res[0] * tangent[i].head<2>();
      auto pts = draw_arc(fn, tangent[i].head<2>(), res[0], midtang, res[1], cornerpt1);
      for (const Vector2d& pt : pts) result.vertices.push_back(pt);

      // 2nd arc
      if (linsystem(Vector3d(midtang[0], midtang[1], 0), tangent[(i + 1) % n], dirz,
                    Vector3d(diff[0], diff[1], 0), res, nullptr))
        printf("prog errr\n");
      Vector2d cornerpt2 = this->points[(i + 1) % n] - res[1] * tangent[(i + 1) % n].head<2>();
      auto pts2 = draw_arc(fn, midtang, res[0], tangent[(i + 1) % n].head<2>(), res[1], cornerpt2);
      for (const Vector2d& pt : pts2) result.vertices.push_back(pt);

      //      result.vertices.push_back(this->points[i]); // Debug pt
    }
  }
  p->addOutline(result);
  p->setColor(*OpenSCAD::parse_color("#f9d72c"));
  return p;
}

static std::shared_ptr<AbstractNode> builtin_polygon(const ModuleInstantiation *inst,
                                                     Arguments arguments)
{
  auto node = std::make_shared<PolygonNode>(inst, CurveDiscretizer(3));

  Parameters parameters =
    Parameters::parse(std::move(arguments), inst->location(), {"points", "paths", "convexity"});

  if (parameters["points"].type() != Value::Type::VECTOR) {
    LOG(message_group::Error, inst->location(), parameters.documentRoot(),
        "Unable to convert points = %1$s to a vector of coordinates",
        parameters["points"].toEchoStringNoThrow());
    return node;
  }
  for (const Value& pointValue : parameters["points"].toVector()) {
    Vector3d point(0, 0, 0);
    if (!pointValue.getVec2(point[0], point[1]) || !std::isfinite(point[0]) ||
        !std::isfinite(point[1])) {
      LOG(message_group::Error, inst->location(), parameters.documentRoot(),
          "Unable to convert points[%1$d] = %2$s to a vec2 of numbers", node->points.size(),
          pointValue.toEchoStringNoThrow());
      node->points.push_back({0, 0, 0});
    } else {
      node->points.push_back(point);
    }
  }

  if (parameters["paths"].type() == Value::Type::VECTOR) {
    size_t pathIndex = 0;
    for (const Value& pathValue : parameters["paths"].toVector()) {
      if (pathValue.type() != Value::Type::VECTOR) {
        LOG(message_group::Error, inst->location(), parameters.documentRoot(),
            "Unable to convert paths[%1$d] = %2$s to a vector of numbers", pathIndex,
            pathValue.toEchoStringNoThrow());
      } else {
        size_t pointIndexIndex = 0;
        std::vector<size_t> path;
        for (const Value& pointIndexValue : pathValue.toVector()) {
          if (pointIndexValue.type() != Value::Type::NUMBER) {
            LOG(message_group::Error, inst->location(), parameters.documentRoot(),
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
    LOG(message_group::Error, inst->location(), parameters.documentRoot(),
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

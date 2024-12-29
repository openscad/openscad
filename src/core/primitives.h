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

#include "geometry/GeometryUtils.h"
#include "geometry/linalg.h"
#include "core/node.h"
#include "core/Parameters.h"

#include <memory>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

template <size_t dimensions>
class Center
{
  bool default_is_centered;
  double default_centers[dimensions];

  bool is_bool{true};
  bool as_bool;

  bool is_valid(double value) { return value == -1 || value == 0 || value == 1; }

  void set_to_default()
  {
    is_bool = true;
    as_bool = default_is_centered;
    for (int i = 0; i < dimensions; i++) {
      as_vect[i] = default_centers[i];
    }
  }

public:
  double as_vect[dimensions];

  Center(bool default_is_centered, const double default_values[dimensions])
    : default_is_centered(default_is_centered)
  {
    for (int i = 0; i < dimensions; i++) {
      default_centers[i] = default_values[i];
    }
  }

  bool parse(const Parameters& parameters)
  {
    if (parameters["center"].type() == Value::Type::BOOL) {
      is_bool = true;
      as_bool = parameters["center"].toBool();
      int common = !as_bool;
      for (double& c : as_vect) {
        c = common;
      }
      return true;
    } else if (parameters["center"].type() == Value::Type::VECTOR) {
      bool okay = true;
      is_bool = false;
      if (parameters["center"].getVec<dimensions>(as_vect)) {
        for (double& c : as_vect) {
          if (!is_valid(c)) {
            okay = false;
            break;
          }
        }
      } else {
        okay = false;
      }
      if (!okay) {
        set_to_default();
        return false;
      }
      return true;
    }
    set_to_default();
    return true;
  }

  std::string toString() const
  {
    std::ostringstream stream;
    if (is_bool) {
      stream << (as_bool ? "true" : "false");
    } else {
      stream << "[";
      for (int i = 0; i < dimensions; i++) {
        if (i) {
          stream << ",";
        }
        stream << as_vect[i];
      }
      stream << "]";
    }
    return stream.str();
  }
};

class CubeNode : public LeafNode
{
  const double c[3] = {1.0, 1.0, 1.0};

public:
  CubeNode(const ModuleInstantiation *mi) : LeafNode(mi), center(false, c) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "cube(size = [" << x << ", " << y << ", " << z << "], "
           << "center = " << center.toString();
    return stream.str();
  }
  std::string name() const override { return "cube"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  double x = 1, y = 1, z = 1;

  Center<3> center;
};

class SphereNode : public LeafNode
{
  const double c[3] = {0.0, 0.0, 0.0};

public:
  SphereNode(const ModuleInstantiation *mi) : LeafNode(mi), center(true, c) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "sphere"
           << "($fn = " << fn
           << ", $fa = " << fa
           << ", $fs = " << fs
           << ", r = " << r
           << ", center = " << center.toString()
           << ")";
    return stream.str();
  }
  std::string name() const override { return "sphere"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  double fn, fs, fa;
  double r = 1;

  Center<3> center;
};


class CylinderNode : public LeafNode
{
  const double c[3] = {0.0, 0.0, 1.0};
public:
  CylinderNode(const ModuleInstantiation *mi) : LeafNode(mi), center(false, c) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "cylinder"
           << "($fn = " << fn
           << ", $fa = " << fa
           << ", $fs = " << fs
           << ", h = " << h
           << ", r1 = " << r1
           << ", r2 = " << r2
           << ", center = " << center.toString()
           << ")";
    return stream.str();
  }
  std::string name() const override { return "cylinder"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  double fn, fs, fa;
  double r1 = 1, r2 = 1, h = 1;

  Center<3> center;
};


class PolyhedronNode : public LeafNode
{
public:
  PolyhedronNode (const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "polyhedron"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  std::vector<Vector3d> points;
  std::vector<IndexedFace> faces;
  int convexity = 1;
};

class SquareNode : public LeafNode
{
  const double c[2] = {1.0, 1.0};

public:
  SquareNode(const ModuleInstantiation *mi) : LeafNode(mi), center(false, c) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "square(size = [" << x << ", " << y << "], center = " << center.toString();
    return stream.str();
  }
  std::string name() const override { return "square"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  double x = 1, y = 1;

  Center<2> center;
};

class CircleNode : public LeafNode
{
  const double c[2] = {0.0, 0.0};

public:
  CircleNode(const ModuleInstantiation *mi) : LeafNode(mi), center(true, c) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "circle"
           << "($fn = " << fn << ", $fa = " << fa << ", $fs = " << fs << ", r = " << r
           << ", center = " << center.toString() << ")";
    return stream.str();
  }
  std::string name() const override { return "circle"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  double fn, fs, fa;
  double r = 1;

  Center<2> center;
};

class PolygonNode : public LeafNode
{
public:
  PolygonNode (const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "polygon"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  std::vector<Vector2d> points;
  std::vector<std::vector<size_t>> paths;
  int convexity = 1;
};

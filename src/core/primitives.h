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

#include "node.h"
#include <sstream>


struct point2d {
  double x, y;
};

struct point3d {
  double x, y, z;
};

class CubeNode : public LeafNode
{
public:
  CubeNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "cube(size = ["
           << x << ", "
           << y << ", "
           << z << "], center = "
           << (center ? "true" : "false") << ")";
    return stream.str();
  }
  std::string name() const override { return "cube"; }
  const Geometry *createGeometry() const override;

  double x = 1, y = 1, z = 1;
  bool center = false;
};


class SphereNode : public LeafNode
{
public:
  SphereNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "sphere"
           << "($fn = " << fn
           << ", $fa = " << fa
           << ", $fs = " << fs
           << ", r = " << r
           << ")";
    return stream.str();
  }
  std::string name() const override { return "sphere"; }
  const Geometry *createGeometry() const override;

  double fn, fs, fa;
  double r = 1;
};


class CylinderNode : public LeafNode
{
public:
  CylinderNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
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
           << ", center = " << (center ? "true" : "false")
           << ")";
    return stream.str();
  }
  std::string name() const override { return "cylinder"; }
  const Geometry *createGeometry() const override;

  double fn, fs, fa;
  double r1 = 1, r2 = 1, h = 1;
  bool center = false;
};


class PolyhedronNode : public LeafNode
{
public:
  PolyhedronNode (const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "polyhedron"; }
  const Geometry *createGeometry() const override;

  std::vector<point3d> points;
  std::vector<std::vector<size_t>> faces;
  int convexity = 1;
};


class SquareNode : public LeafNode
{
public:
  SquareNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "square(size = ["
           << x << ", "
           << y << "], center = "
           << (center ? "true" : "false") << ")";
    return stream.str();
  }
  std::string name() const override { return "square"; }
  const Geometry *createGeometry() const override;

  double x = 1, y = 1;
  bool center = false;
};


class CircleNode : public LeafNode
{
public:
  CircleNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "circle"
           << "($fn = " << fn
           << ", $fa = " << fa
           << ", $fs = " << fs
           << ", r = " << r
           << ")";
    return stream.str();
  }
  std::string name() const override { return "circle"; }
  const Geometry *createGeometry() const override;

  double fn, fs, fa;
  double r = 1;
};


class PolygonNode : public LeafNode
{
public:
  PolygonNode (const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "polygon"; }
  const Geometry *createGeometry() const override;

  std::vector<point2d> points;
  std::vector<std::vector<size_t>> paths;
  int convexity = 1;
};


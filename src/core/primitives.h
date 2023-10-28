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
#pragma once

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
  CubeNode() : CubeNode(new ModuleInstantiation("cube")) {}
  CubeNode(ModuleInstantiation *mi) : LeafNode(mi) {}
  CubeNode(double size, bool center = false) : CubeNode(size, size, size, center) {}
  CubeNode(double x, double y, double z, bool center = false) : CubeNode() {
    this->x = x;
    this->y = y; 
    this->z = z; 
    this->center = center;
  }
  static std::shared_ptr<CubeNode> cube() { return std::make_shared<CubeNode>(); }
  static std::shared_ptr<CubeNode> cube(double size) {
    return std::make_shared<CubeNode>(size); 
  }
  static std::shared_ptr<CubeNode> cube(double size, bool center) { //= false
    return std::make_shared<CubeNode>(size, center); 
  }
  static std::shared_ptr<CubeNode> cube(double x, double y, double z) { 
    return std::make_shared<CubeNode>(x, y, z); 
  }
  static std::shared_ptr<CubeNode> cube(double x, double y, double z, bool center) { 
    return std::make_shared<CubeNode>(x, y, z, center); 
  }
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
  SphereNode() : SphereNode(new ModuleInstantiation("sphere")) {}
  SphereNode(ModuleInstantiation *mi) : LeafNode(mi) {}
  SphereNode(double r) : SphereNode() { this-> r = r; }
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
  static std::shared_ptr<SphereNode> sphere() { return std::make_shared<SphereNode>(); }
  static std::shared_ptr<SphereNode> sphere(double r) {
    return std::make_shared<SphereNode>(r); 
  }

  double fn, fs, fa;
  double r = 1;
};


class CylinderNode : public LeafNode
{
public:
  CylinderNode() : CylinderNode(new ModuleInstantiation("cylinder")) {}
  CylinderNode(ModuleInstantiation *mi) : LeafNode(mi) {}
  CylinderNode(double r, double h, bool center = false) : CylinderNode(r, r, h, center) {}
  CylinderNode(double r1, double r2, double h, bool center = false) : CylinderNode() {
    this->r1 = r1;
    this->r2 = r2;
    this->h = h;
    this->center = center;
  }
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
  static std::shared_ptr<CylinderNode> cylinder() { return std::make_shared<CylinderNode>(); }
  static std::shared_ptr<CylinderNode> cylinder(double r, double h) { 
    return std::make_shared<CylinderNode>(r, h); 
  }
  static std::shared_ptr<CylinderNode> cylinder(double r, double h, bool center) { 
    return std::make_shared<CylinderNode>(r, h, center); 
  }
  static std::shared_ptr<CylinderNode> cylinder(double r1, double r2, double h) { 
    return std::make_shared<CylinderNode>(r1, r2, h); 
  }
  static std::shared_ptr<CylinderNode> cylinder(double r1, double r2, double h, bool center) { 
    return std::make_shared<CylinderNode>(r1, r2, h, center); 
  }

  double fn, fs, fa;
  double r1 = 1, r2 = 1, h = 1;
  bool center = false;
};


class PolyhedronNode : public LeafNode
{
public:
  PolyhedronNode() : PolyhedronNode(new ModuleInstantiation("polyhedron")) {}
  PolyhedronNode (ModuleInstantiation *mi) : LeafNode(mi) {}
  PolyhedronNode(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces, int convexity = 1) : PolyhedronNode() {
    this->points = points;
    this->faces = faces;
    this->convexity = convexity;
  }
  std::string toString() const override;
  std::string name() const override { return "polyhedron"; }
  const Geometry *createGeometry() const override;
  static shared_ptr<PolyhedronNode> polyhedron() { return std::make_shared<PolyhedronNode>();}
  static shared_ptr<PolyhedronNode> polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces) 
  { 
    return std::make_shared<PolyhedronNode>(points, faces);
  }
  static shared_ptr<PolyhedronNode> polyhedron(std::vector<point3d>& points, std::vector<std::vector<size_t>>& faces, int convexity) {
    return std::make_shared<PolyhedronNode>(points, faces, convexity);
  }

  std::vector<point3d> points;
  std::vector<std::vector<size_t>> faces;
  int convexity = 1;
};


class SquareNode : public LeafNode
{
public:
  SquareNode() : SquareNode(new ModuleInstantiation("square")) {}
  SquareNode(ModuleInstantiation *mi) : LeafNode(mi) {}
  SquareNode(double x, bool center = false) : SquareNode(x, x, center) {}
  SquareNode(double x, double y, bool center = false) : SquareNode() 
  {
    this->x = x;
    this->y = y;
    this->center = center;
  }
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
  static shared_ptr<SquareNode> square() { return std::make_shared<SquareNode>(); }
  static shared_ptr<SquareNode> square(double x) { return std::make_shared<SquareNode>(x); }
  static shared_ptr<SquareNode> square(double x, bool center) { 
    return std::make_shared<SquareNode>(x, center); 
  }
  static shared_ptr<SquareNode> square(double x, double y) { return std::make_shared<SquareNode>(x, y); }
  static shared_ptr<SquareNode> square(double x, double y, bool center) { 
    return std::make_shared<SquareNode>(x, y, center); 
  }

  double x = 1, y = 1;
  bool center = false;
};


class CircleNode : public LeafNode
{
public:
  CircleNode() : CircleNode(new ModuleInstantiation("square")) {}
  CircleNode(ModuleInstantiation *mi) : LeafNode(mi) {}
  CircleNode(double r) : CircleNode() { this->r = r; }
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
  static shared_ptr<CircleNode> circle() { return std::make_shared<CircleNode>(); }
  static shared_ptr<CircleNode> circle(double r) { return std::make_shared<CircleNode>(r); }

  double fn, fs, fa;
  double r = 1;
};


class PolygonNode : public LeafNode
{
public:
  PolygonNode() : PolygonNode(new ModuleInstantiation("polygon")) {}
  PolygonNode(ModuleInstantiation *mi) : LeafNode(mi) {}
  PolygonNode(std::vector<point2d>& points) : PolygonNode() 
  {
    this->points = points;
  }
  PolygonNode(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths) : PolygonNode() 
  {
    this->points = points;
    this->paths = paths;
  }
  PolygonNode(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths, int convexity) : PolygonNode()
  {
    this->points = points;
    this->paths = paths;
    this->convexity = convexity;
  }
  std::string toString() const override;
  std::string name() const override { return "polygon"; }
  const Geometry *createGeometry() const override;
  static shared_ptr<PolygonNode> polygon() { return std::make_shared<PolygonNode>();}
  static shared_ptr<PolygonNode> polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths) 
  { 
    return std::make_shared<PolygonNode>(points, paths);
  }
  static shared_ptr<PolygonNode> polygon(std::vector<point2d>& points, std::vector<std::vector<size_t>>& paths, int convexity) {
    return std::make_shared<PolygonNode>(points, paths, convexity);
  }

  std::vector<point2d> points;
  std::vector<std::vector<size_t>> paths;
  int convexity = 1;
};


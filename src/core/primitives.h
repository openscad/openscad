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

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/CurveDiscretizer.h"
#include "core/ModuleInstantiation.h"
#include "core/node.h"
#include "geometry/Geometry.h"
#include "geometry/GeometryUtils.h"
#include "geometry/linalg.h"

class CubeNode : public LeafNode
{
public:
  CubeNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "cube(size = [" << dim[0] << ", " << dim[1] << ", " << dim[2] << "], center = ";
    if (center[0] == center[1] && center[1] == center[2])
      stream << ((center[0] == 0) ? "true" : "false");
    else {
      stream << "\"";
      for (int i = 0; i < 3; i++) {
        if (center[i] < 0) stream << "-";
        if (center[i] == 0) stream << "|";
        if (center[i] > 0) stream << "+";
      }
      stream << "\"";
    }
    stream << ")";
    return stream.str();
  }
  std::string name() const override { return "cube"; }
  std::unique_ptr<const Geometry> createGeometry() const override;
  virtual std::shared_ptr<const Geometry> dragPoint(const Vector3d& pt, const Vector3d& delta,
                                                    DragResult& result) override;

  double dim[3] = {1, 1, 1};
  int dragflags = 0;  // X, Y, Z
  double dim_[3] = {0, 0, 0};
  int center[3] = {1, 1, 1};  // -1 means negative side, 0 means centered, 1 means positive side
};

std::unique_ptr<const Geometry> sphereCreateFuncGeometry(void *funcptr, double fs, int n);

class SphereNode : public LeafNode
{
public:
  SphereNode(const ModuleInstantiation *mi, CurveDiscretizer discretizer)
    : LeafNode(mi), discretizer(std::move(discretizer))
  {
  }
  std::string toString() const override;
  std::string name() const override { return "sphere"; }
  std::unique_ptr<const Geometry> createGeometry() const override;
  virtual std::shared_ptr<const Geometry> dragPoint(const Vector3d& pt, const Vector3d& delta,
                                                    DragResult& result) override;

  CurveDiscretizer discretizer;
  double r = 1;
#ifdef ENABLE_PYTHON
  void *r_func = nullptr;
#endif
  int dragflags = 0;  // r
};

class CylinderNode : public LeafNode
{
public:
  CylinderNode(const ModuleInstantiation *mi, CurveDiscretizer discretizer)
    : LeafNode(mi), discretizer(std::move(discretizer))
  {
  }
  std::string toString() const override;
  std::string name() const override { return "cylinder"; }
  std::unique_ptr<const Geometry> createGeometry() const override;
  virtual std::shared_ptr<const Geometry> dragPoint(const Vector3d& pt, const Vector3d& delta,
                                                    DragResult& result) override;

  CurveDiscretizer discretizer;
  double r1 = 1, r2 = 1, h = 1;
  double angle = 360;
  bool center = false;
  int dragflags = 0;  // r, h
};

class PolyhedronNode : public LeafNode
{
public:
  PolyhedronNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "polyhedron"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  std::vector<Vector3d> points;
  std::vector<IndexedFace> faces;
  int convexity = 1;
  std::vector<int32_t> color_indices;  // when present, must be same size as faces
  std::vector<Color4f> colors;
};

class EdgeNode : public LeafNode
{
public:
  EdgeNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "edge(size = " << size << ", center = " << (center ? "true" : "false") << ")";
    return stream.str();
  }
  std::string name() const override { return "edge"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  double size = 1;
  bool center = false;
};

class SquareNode : public LeafNode
{
public:
  SquareNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override
  {
    std::ostringstream stream;
    stream << "square(size = [" << x << ", " << y << "], center = " << (center ? "true" : "false")
           << ")";
    return stream.str();
  }
  std::string name() const override { return "square"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  double x = 1, y = 1;
  bool center = false;
};

class CircleNode : public LeafNode
{
public:
  CircleNode(const ModuleInstantiation *mi, CurveDiscretizer discretizer)
    : LeafNode(mi), discretizer(std::move(discretizer))
  {
  }
  std::string toString() const override;
  std::string name() const override { return "circle"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  CurveDiscretizer discretizer;
  double r = 1;
  double angle = 360;
};

class PolygonNode : public LeafNode
{
public:
  PolygonNode(const ModuleInstantiation *mi, CurveDiscretizer discretizer)
    : LeafNode(mi), discretizer(std::move(discretizer))
  {
  }
  std::string toString() const override;
  std::string name() const override { return "polygon"; }
  std::unique_ptr<const Geometry> createGeometry() const override;

  std::vector<Vector3d> points;
  std::vector<std::vector<size_t>> paths;
  int convexity = 1;
  VectorOfVector2d createGeometry_sub(const std::vector<Vector3d>& points,
                                      const std::vector<size_t>& path) const;
  CurveDiscretizer discretizer;
};

class SplineNode : public LeafNode
{
public:
  SplineNode(const ModuleInstantiation *mi) : LeafNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "spline"; }
  std::vector<Vector2d> draw_arc(int fn, const Vector2d& tang1, double l1, const Vector2d& tang2,
                                 double l2, const Vector2d& cornerpt) const;
  std::unique_ptr<const Geometry> createGeometry() const override;

  std::vector<Vector2d> points;
  double fn, fa, fs;
};

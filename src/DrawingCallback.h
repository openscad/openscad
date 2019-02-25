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

#include <vector>
#include <math.h>
#include <Eigen/Core>
#include "Polygon2d.h"

class DrawingCallback
{
public:
  DrawingCallback(unsigned long fn);
  virtual ~DrawingCallback();

  void start_glyph();
  void finish_glyph();
  void set_glyph_offset(double offset_x, double offset_y);
  void add_glyph_advance(double advance_x, double advance_y);
  std::vector<const Geometry *> get_result();

  void move_to(const Vector2d &to);
  void line_to(const Vector2d &to);
  void curve_to(const Vector2d &c1, const Vector2d &to);
  void curve_to(const Vector2d &c1, const Vector2d &c2, const Vector2d &to);
private:
  Vector2d pen;
  Vector2d offset;
  Vector2d advance;
  unsigned long fn;

  Outline2d outline;
  class Polygon2d *polygon;
  std::vector<const class Geometry *> polygons;

  void add_vertex(const Vector2d &v);
};

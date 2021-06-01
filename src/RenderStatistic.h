/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2020 Golubev Alexander <fatzer2@gmail.com>
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

#ifndef RENDERSTATISTIC_H
#define RENDERSTATISTIC_H

#include "Geometry.h"

#include <chrono>

/**
 * An utility class to collect and print rendering statistics for the given
 * geometry
 */
class RenderStatistic: protected GeometryVisitor
{
public:
  /**
   * Construct a statistic printer for the given geometry
   */
  RenderStatistic() {};
  
  /**
   * Print some statistic on cache usage. Namely, stats on the @ref GeometryCache
   * and @ref CGALCache (if enabled).
   */
  static void printCacheStatistic();
  
  /**
   * Format and print time elapsed by rendering.
   * @arg time elapsed by rendering in seconds
   */
  static void printRenderingTime(std::chrono::milliseconds ms);
  
  /**
   * Actually print the statistic based on the given Geometry
   * @arg geom A Geometry-derived object statistic for which we should print.
   */
  void print(const Geometry &geom);
  
protected:
  void visit(const class GeometryList &node) override;
  void visit(const class PolySet &node) override;
  void visit(const class Polygon2d &node) override;
#ifdef ENABLE_CGAL
  void visit(const class CGAL_Nef_polyhedron &node) override;
#endif // ENABLE_CGAL
};

#endif // RENDERSTATISTIC_H

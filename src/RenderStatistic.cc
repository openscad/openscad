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

#include "printutils.h"
#include "GeometryCache.h"
#include "CGALCache.h"
#include "polyset.h"
#include "Polygon2d.h"

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#endif // ENABLE_CGAL

#include "RenderStatistic.h"


void RenderStatistic::printCacheStatistic()
{
  GeometryCache::instance()->print();
#ifdef ENABLE_CGAL
  CGALCache::instance()->print();
#endif
}

void RenderStatistic::printRenderingTime(std::chrono::milliseconds ms)
{
  PRINTB("Total rendering time: %d:%02d:%02d.%03d",
    (ms.count() /1000/60/60     ) %
    (ms.count() /1000/60 % 60   ) %
    (ms.count() /1000    % 60   ) %
    (ms.count()          % 1000 ) 
  );
}

void RenderStatistic::print(const Geometry *geom)
{
  geom->accept(*this);
}

void RenderStatistic::visit(const GeometryList& geomlist)
{
  PRINT("   Top level object is a list of objects:");
  PRINTB("   Objects:     %d", geomlist.getChildren().size());
}

void RenderStatistic::visit(const PolySet& ps)
{
  assert(ps.getDimension() == 3);
  PRINT("   Top level object is a 3D object:");
  PRINTB("   Facets:     %6d", ps.numFacets());
}

void RenderStatistic::visit(const Polygon2d& poly)
{
  PRINT("   Top level object is a 2D object:");
  PRINTB("   Contours:     %6d", poly.outlines().size());
}

#ifdef ENABLE_CGAL
void RenderStatistic::visit(const CGAL_Nef_polyhedron& Nef)
{
  if (Nef.getDimension() == 3) {
    bool simple = Nef.p3->is_simple();
    PRINT("   Top level object is a 3D object:");
    PRINTB("   Simple:     %6s", (simple ? "yes" : "no"));
    PRINTB("   Vertices:   %6d", Nef.p3->number_of_vertices());
    PRINTB("   Halfedges:  %6d", Nef.p3->number_of_halfedges());
    PRINTB("   Edges:      %6d", Nef.p3->number_of_edges());
    PRINTB("   Halffacets: %6d", Nef.p3->number_of_halffacets());
    PRINTB("   Facets:     %6d", Nef.p3->number_of_facets());
    PRINTB("   Volumes:    %6d", Nef.p3->number_of_volumes());
    if (!simple) {
      PRINT("UI-WARNING: Object may not be a valid 2-manifold and may need repair!");
    }
  }
}
#endif // ENABLE_CGAL

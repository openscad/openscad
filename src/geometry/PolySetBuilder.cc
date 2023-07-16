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

#include "PolySetBuilder.h"
#include <PolySet.h>


// https://hackernoon.com/design-patterns-builder-pattern-in-modern-c-x1283uy3
PolySetBuilder::PolySetBuilder(int dim, int vertices_count)
{
	ps= new PolySet(dim, true);
	ps->indices.reserve(vertices_count);
}

int PolySetBuilder::vertexIndex(const Vector3d &pt)
{
  int ind;
  if(pointMap.count(pt) == 0) {
    ind=ps->vertices.size();
    ps->vertices.push_back(pt);
    pointMap[pt]=ind;
  } else ind=pointMap[pt];
  return ind;
}
void PolySetBuilder::append_poly(const std::vector<int> &inds)
{
  ps->indices.push_back(inds);        
  ps->dirty = true;

}
PolySet *PolySetBuilder::result(void)
{
	return ps;
}


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


PolySetBuilder::PolySetBuilder(int vertices_count, int indices_count, int dim, bool convex)
{
  ps= std::make_shared<PolySet>(dim);
  ps->convex=convex;
  if(vertices_count != 0) ps->vertices.reserve(vertices_count);
  if(indices_count != 0) ps->indices.reserve(indices_count);
}

int PolySetBuilder::vertexIndex(const Vector3d &pt)
{
  return allVertices.lookup(pt);
}

void PolySetBuilder::append_poly(const std::vector<int> &inds)
{
  ps->indices.push_back(inds);        
  ps->dirty = true;
}

void PolySetBuilder::append_poly(const std::vector<Vector3d> &v)
{
  std::vector<int> inds;
  for(const auto &pt: v)
    inds.push_back(vertexIndex(pt));  
  ps->indices.push_back(inds);        
  ps->dirty = true;
}

void PolySetBuilder::append_poly(int nvertices){
  ps->indices.emplace_back().reserve(nvertices);
}

void PolySetBuilder::append_vertex(int ind){
  ps->indices.back().push_back(ind);
  ps->dirty = true;
}

void PolySetBuilder::prepend_vertex(int ind){
  ps->indices.back().insert(ps->indices.back().begin(), ind);
  ps->dirty = true;
}

int PolySetBuilder::numVertices(void) {
	return allVertices.size();
}

void PolySetBuilder::append(PolySet *ps)
{
  for(const auto &poly : ps->indices) {
    append_poly(poly.size());
    for(const auto &ind: poly) {
      append_vertex(vertexIndex(ps->vertices[ind]));
    }
  }
}
void PolySetBuilder::reset(void) {
}
std::shared_ptr<PolySet> PolySetBuilder::result(void)
{
  ps->vertices = allVertices.getArray();
  return ps;
}

void PolySetBuilder::setConvexity(int conv){
	ps->convex=conv;
}


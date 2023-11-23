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
#include "Geometry.h"
#include "cgalutils.h"


#include <memory>

#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#endif


#include "CGAL_Nef_polyhedron.h"
#include "CGALHybridPolyhedron.h"

PolySetBuilder::PolySetBuilder(int vertices_count, int indices_count, int dim, boost::tribool convex)
{
  ps= new PolySet(dim, convex);
  if(vertices_count != 0) ps->vertices.reserve(vertices_count);
  if(indices_count != 0) ps->indices.reserve(indices_count);
}

PolySetBuilder::PolySetBuilder(const Polygon2d pol)
{
  ps= new PolySet(pol);
  ps->dirty=false;
}

int PolySetBuilder::vertexIndex(const Vector3d &pt)
{
  return allVertices.lookup(pt);
}

void PolySetBuilder::appendPoly(const std::vector<int> &inds)
{
  ps->indices.push_back(inds);        
}

void PolySetBuilder::appendGeometry(const shared_ptr<const Geometry>& geom)
{
  if (const auto geomlist = dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const Geometry::GeometryItem& item : geomlist->getChildren()) {
      appendGeometry(item.second);
    }
  } else if (const auto N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    PolySet ps(3);
    bool err = CGALUtils::createPolySetFromNefPolyhedron3(*(N->p3), ps);
    if (err) {
      LOG(message_group::Error, "Nef->PolySet failed");
    } else {
      append(&ps);
    }
  } else if (const auto ps = dynamic_pointer_cast<const PolySet>(geom)) {
    append(ps.get());
  } else if (const auto hybrid = dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    // TODO(ochafik): Implement appendGeometry(Surface_mesh) instead of converting to PolySet
    append(hybrid->toPolySet().get());
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
   append(mani->toPolySet().get());
#endif
  } else if (dynamic_pointer_cast<const Polygon2d>(geom)) { // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else { // NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }

}


void PolySetBuilder::appendPoly(const std::vector<Vector3d> &v)
{
  std::vector<int> inds;
  inds.reserve(v.size());
  for(const auto &pt: v)
    inds.push_back(vertexIndex(pt));  
  ps->indices.push_back(inds);        
}

void PolySetBuilder::appendPoly(int nvertices){
  ps->indices.emplace_back().reserve(nvertices);
}

void PolySetBuilder::appendVertex(int ind){
  ps->indices.back().push_back(ind);
}

void PolySetBuilder::prependVertex(int ind){
  ps->indices.back().insert(ps->indices.back().begin(), ind);
}

int PolySetBuilder::numVertices(void) {
	return allVertices.size();
}

void PolySetBuilder::append(const PolySet *ps)
{
  for(const auto &poly : ps->indices) {
    appendPoly(poly.size());
    for(const auto &ind: poly) {
      appendVertex(vertexIndex(ps->vertices[ind]));
    }
  }
}

PolySet *PolySetBuilder::build(void)
{
  ps->dirty = true;
  ps->vertices = allVertices.getArray();
  return ps;
}

void PolySetBuilder::setConvexity(int convexity){
	ps->convexity=convexity;
}


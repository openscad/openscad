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

#ifdef ENABLE_CGAL
#include "cgalutils.h"
#include "CGAL_Nef_polyhedron.h"
#include "CGALHybridPolyhedron.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#endif

PolySetBuilder::PolySetBuilder(int vertices_count, int indices_count, int dim, boost::tribool convex)
  : convex_(convex), dim_(dim)
{
  if (vertices_count != 0) vertices_.reserve(vertices_count);
  if (indices_count != 0) indices_.reserve(indices_count);
}

void PolySetBuilder::setConvexity(int convexity){
  convexity_ = convexity;
}

int PolySetBuilder::numVertices() const {
  return vertices_.size();
}

int PolySetBuilder::vertexIndex(const Vector3d& pt)
{
  return vertices_.lookup(pt);
}

void PolySetBuilder::appendPoly(const std::vector<int>& inds)
{
  auto& face = indices_.emplace_back();
  face.insert(face.begin(), inds.begin(), inds.end());
}

void PolySetBuilder::appendGeometry(const std::shared_ptr<const Geometry>& geom)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const Geometry::GeometryItem& item : geomlist->getChildren()) {
      appendGeometry(item.second);
    }
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    append(*ps);
#ifdef ENABLE_CGAL
  } else if (const auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    if (const auto ps = CGALUtils::createPolySetFromNefPolyhedron3(*(N->p3))) {
      append(*ps);
    }
    else {
      LOG(message_group::Error, "Nef->PolySet failed");
    }
  } else if (const auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    // TODO(ochafik): Implement appendGeometry(Surface_mesh) instead of converting to PolySet
    append(*hybrid->toPolySet());
#endif // ifdef ENABLE_CGAL
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    append(*mani->toPolySet());
#endif
  } else if (std::dynamic_pointer_cast<const Polygon2d>(geom)) { // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported geometry");
  } else { // NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }

}


void PolySetBuilder::appendPoly(const std::vector<Vector3d>& v)
{
  IndexedFace inds;
  inds.reserve(v.size());
  for (const auto& pt: v)
    inds.push_back(vertexIndex(pt));
  indices_.push_back(inds);
}

void PolySetBuilder::appendPoly(int nvertices){
  indices_.emplace_back().reserve(nvertices);
}

void PolySetBuilder::appendVertex(int ind)
{
  indices_.back().push_back(ind);
}

void PolySetBuilder::appendVertex(const Vector3d &v)
{
  appendVertex(vertexIndex(v));
}

void PolySetBuilder::prependVertex(int ind)
{
  indices_.back().insert(indices_.back().begin(), ind);
}

void PolySetBuilder::prependVertex(const Vector3d &v)
{
  prependVertex(vertexIndex(v));
}

void PolySetBuilder::append(const PolySet& ps)
{
  for (const auto& poly : ps.indices) {
    appendPoly(poly.size());
    for (const auto& ind: poly) {
      appendVertex(vertexIndex(ps.vertices[ind]));
    }
  }
}

std::unique_ptr<PolySet> PolySetBuilder::build()
{
  std::unique_ptr<PolySet> polyset;
  polyset = std::make_unique<PolySet>(dim_, convex_);
  vertices_.copy(std::back_inserter(polyset->vertices));
  polyset->indices = std::move(indices_);
  polyset->setConvexity(convexity_);
  polyset->isTriangular = true;
  for (const auto& face : polyset->indices) {
    if (face.size() > 3) {
      polyset->isTriangular = false;
      break;
    }
  }
  return polyset;
}

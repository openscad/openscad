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

void PolySetBuilder::appendGeometry(const std::shared_ptr<const Geometry>& geom)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const Geometry::GeometryItem& item : geomlist->getChildren()) {
      appendGeometry(item.second);
    }
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    appendPolySet(*ps);
#ifdef ENABLE_CGAL
  } else if (const auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    if (const auto ps = CGALUtils::createPolySetFromNefPolyhedron3(*(N->p3))) {
      appendPolySet(*ps);
    }
    else {
      LOG(message_group::Error, "Nef->PolySet failed");
    }
  } else if (const auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    // TODO(ochafik): Implement appendGeometry(Surface_mesh) instead of converting to PolySet
    appendPolySet(*hybrid->toPolySet());
#endif // ifdef ENABLE_CGAL
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    appendPolySet(*mani->toPolySet());
#endif
  } else if (std::dynamic_pointer_cast<const Polygon2d>(geom)) { // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported geometry");
  } else { // NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }

}

void PolySetBuilder::appendPolygon(const std::vector<int>& inds)
{
  beginPolygon(inds.size());
  for (int idx : inds) addVertex(idx);
  endPolygon();
}

void PolySetBuilder::appendPolygon(const std::vector<Vector3d>& polygon)
{
  beginPolygon(polygon.size());
  for (const auto& v: polygon) addVertex(v);
  endPolygon();
}

void PolySetBuilder::beginPolygon(int nvertices) {
  endPolygon();
  current_polygon_.reserve(nvertices);
}

void PolySetBuilder::addVertex(int ind)
{
  // Ignore consecutive duplicate indices
  if (current_polygon_.empty() || 
      ind != current_polygon_.back() && ind != current_polygon_.front()) {
    current_polygon_.push_back(ind);
  }
}

void PolySetBuilder::addVertex(const Vector3d &v)
{
  addVertex(vertexIndex(v));
}

void PolySetBuilder::endPolygon() {
  // FIXME: Should we check for self-touching polygons (non-consecutive duplicate indices)?

  // FIXME: Can we move? What would the state of current_polygon_ be after move?
  if (current_polygon_.size() >= 3) {
    indices_.push_back(current_polygon_);
  }
  current_polygon_.clear();
}

void PolySetBuilder::appendPolySet(const PolySet& ps)
{
  for (const auto& poly : ps.indices) {
    beginPolygon(poly.size());
    for (const auto& ind: poly) {
      addVertex(ps.vertices[ind]);
    }
    endPolygon();
  }
}

std::unique_ptr<PolySet> PolySetBuilder::build()
{
  endPolygon();
  std::unique_ptr<PolySet> polyset;
  polyset = std::make_unique<PolySet>(dim_, convex_);
  vertices_.copy(std::back_inserter(polyset->vertices));
  polyset->indices = std::move(indices_);
  polyset->setConvexity(convexity_);
  bool is_triangular = true;
  for (const auto& face : polyset->indices) {
    if (face.size() > 3) {
      is_triangular = false;
      break;
    }
  }
  polyset->setTriangular(is_triangular);
  return polyset;
}

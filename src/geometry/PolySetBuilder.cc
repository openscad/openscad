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

#include "geometry/PolySetBuilder.h"
#include "geometry/PolySet.h"
#include "geometry/Geometry.h"

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#include "geometry/cgal/CGALHybridPolyhedron.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif

#include <algorithm>
#include <iterator>
#include <cassert>
#include <utility>
#include <cstdint>
#include <memory>
#include <vector>

PolySetBuilder::PolySetBuilder(int vertices_count, int indices_count, int dim, boost::tribool convex)
  : convex_(convex), dim_(dim)
{
  reserve(vertices_count, indices_count);
}

void PolySetBuilder::reserve(int vertices_count, int indices_count) {
  if (vertices_count != 0) vertices_.reserve(vertices_count);
  if (indices_count != 0) indices_.reserve(indices_count);
}

void PolySetBuilder::setConvexity(int convexity){
  convexity_ = convexity;
}

int PolySetBuilder::numVertices() const {
  return vertices_.size();
}

int PolySetBuilder::numPolygons() const {
  return indices_.size();
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
  // Copy color indices lazily.
  if (!ps.color_indices.empty()) {
    // If we hadn't built color_indices_ yet, catch up / fill w/ -1.
    if (color_indices_.empty() && !indices_.empty()) {
      color_indices_.resize(indices_.size(), -1);
    }
    color_indices_.reserve(color_indices_.size() + ps.color_indices.size());

    auto nColors = ps.colors.size();
    std::vector<uint32_t> color_map(nColors);
    for (int i = 0; i < nColors; i++) {
      const auto& color = ps.colors[i];
      // Find index of color in colors_, or add it if it doesn't exist
      auto it = std::find(colors_.begin(), colors_.end(), color);
      if (it == colors_.end()) {
        color_map[i] = colors_.size();
        colors_.push_back(color);
      } else {
        color_map[i] = it - colors_.begin();
      }
    }
    for (auto color_index : ps.color_indices) {
      color_indices_.push_back(color_index < 0 ? -1 : color_map[color_index]);
    }
  } else if (!color_indices_.empty()) {
    // If we already built color_indices_ but don't have colors with this ps, fill with -1.
    color_indices_.resize(color_indices_.size() + ps.indices.size(), -1);
  }

  reserve(numVertices() + ps.vertices.size(), numPolygons() + ps.indices.size());
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
  polyset->color_indices = std::move(color_indices_);
  polyset->colors = std::move(colors_);
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

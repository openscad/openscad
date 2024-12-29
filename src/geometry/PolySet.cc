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

#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "geometry/linalg.h"
#include "utils/printutils.h"
#include "geometry/Grid.h"
#include <algorithm>
#include <sstream>
#include <memory>
#include <Eigen/LU>
#include <cstddef>
#include <string>
#include <vector>

/*! /class PolySet

   The PolySet class fulfils multiple tasks, partially for historical reasons.
   FIXME: It's a bit messy and is a prime target for refactoring.

   1) Store 2D and 3D polygon meshes from all origins
   2) Store 2D outlines, used for rendering edges (2D only)
   3) Rendering of polygons and edges


   PolySet must only contain convex polygons

 */

PolySet::PolySet(unsigned int dim, boost::tribool convex)
 : dim_(dim), convex_(convex)
{
}

std::unique_ptr<Geometry> PolySet::copy() const {
  return std::make_unique<PolySet>(*this);
}

std::string PolySet::dump() const
{
  std::ostringstream out;
  out << "PolySet:"
      << "\n dimensions:" << dim_
      << "\n convexity:" << this->convexity
      << "\n num polygons: " << indices.size()
      << "\n polygons data:";
  for (const auto& polygon : indices) {
    out << "\n  polygon begin:";
    for (auto v : polygon) {
      out << "\n   vertex:" << this->vertices[v].transpose();
    }
  }
  out << "\nPolySet end";
  return out.str();
}

BoundingBox PolySet::getBoundingBox() const
{
  if (bbox_.isNull()) {
    for (const auto& v : vertices) {
      bbox_.extend(v);
    }
  }
  return bbox_;
}

size_t PolySet::memsize() const
{
  size_t mem = 0;
  for (const auto& p : this->indices) mem += p.size() * sizeof(int);
  for (const auto& p : this->vertices) mem += p.size() * sizeof(Vector3d);
  mem += sizeof(PolySet);
  return mem;
}
void PolySet::transform(const Transform3d& mat)
{
  // If mirroring transform, flip faces to avoid the object to end up being inside-out
  bool mirrored = mat.matrix().determinant() < 0;

  for (auto& v : this->vertices)
      v = mat * v;

  if(mirrored)
    for (auto& p : this->indices) {
      std::reverse(p.begin(), p.end());
  }
  bbox_.setNull();
}

void PolySet::setColor(const Color4f& c) {
  colors = {c};
  color_indices.assign(indices.size(), 0);
}

bool PolySet::isConvex() const {
  if (convex_ || this->isEmpty()) return true;
  if (!convex_) return false;
  bool is_convex = PolySetUtils::is_approximately_convex(*this);
  convex_ = is_convex;
  return is_convex;
}

void PolySet::resize(const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize)
{
  this->transform(GeometryUtils::getResizeTransform(this->getBoundingBox(), newsize, autosize));
}

/*!
   Quantizes vertices by gridding them as well as merges close vertices belonging to
   neighboring grids.
   May reduce the number of polygons if polygons collapse into < 3 vertices.
 */
void PolySet::quantizeVertices(std::vector<Vector3d> *pPointsOut)
{
  Grid3d<unsigned int> grid(GRID_FINE);
  std::vector<unsigned int> indices; // Vertex indices in one polygon
  for (size_t i=0; i < this->indices.size();) {
    IndexedFace& ind_f = this->indices[i];
    indices.resize(ind_f.size());
    // Quantize all vertices. Build index list
    for (unsigned int i = 0; i < ind_f.size(); ++i) {
      indices[i] = grid.align(this->vertices[ind_f[i]]);
      if (pPointsOut && pPointsOut->size() < grid.db.size()) {
        pPointsOut->push_back(this->vertices[ind_f[i]]);
      }
    }
    // Remove consecutive duplicate vertices
    auto currp = ind_f.begin();
    for (unsigned int i = 0; i < indices.size(); ++i) {
      if (indices[i] != indices[(i + 1) % indices.size()]) {
        (*currp++) = ind_f[i];
      }
    }
    ind_f.erase(currp, ind_f.end());
    if (ind_f.size() < 3) {
      PRINTD("Removing collapsed polygon due to quantizing");
      this->indices.erase(this->indices.begin()+i);
    } else {
      i++;
    }
  }
}

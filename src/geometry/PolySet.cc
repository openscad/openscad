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

#include "PolySet.h"
#include "PolySetUtils.h"
#include "linalg.h"
#include "printutils.h"
#include "Grid.h"
#include <Eigen/LU>
#include <utility>

/*! /class PolySet

   The PolySet class fulfils multiple tasks, partially for historical reasons.
   FIXME: It's a bit messy and is a prime target for refactoring.

   1) Store 2D and 3D polygon meshes from all origins
   2) Store 2D outlines, used for rendering edges (2D only)
   3) Rendering of polygons and edges


   PolySet must only contain convex polygons

 */

PolySet::PolySet(unsigned int dim, boost::tribool convex) : dim(dim), convex(convex), dirty(false)
{
}

PolySet::PolySet(Polygon2d origin) : polygon(std::move(origin)), dim(2), convex(unknown), dirty(false)
{
}

std::string PolySet::dump() const
{
  std::ostringstream out;
  out << "PolySet:"
      << "\n dimensions:" << this->dim
      << "\n convexity:" << this->convexity
      << "\n num polygons: " << polygons_ind.size()
      << "\n num outlines: " << polygon.outlines().size()
      << "\n polygons data:";
  for (const auto& polygon : polygons_ind) {
    out << "\n  polygon begin:";
    for (auto v : polygon) {
      out << "\n   vertex:" << this->points[v].transpose();
    }
  }
  out << "\n outlines data:";
  out << polygon.dump();
  out << "\nPolySet end";
  return out.str();
}

void PolySet::append_poly(size_t expected_vertex_count)
{
  polygons_ind.emplace_back().reserve(expected_vertex_count);
}

int PolySet::pointIndex(const Vector3f &ptx)
{
  int ind;
  Vector3d pt{ptx[0],ptx[1],ptx[2]};
  if(pointMap.count(pt) == 0) {
    ind=this->points.size();
    this->points.push_back(pt);
    pointMap[pt]=ind;
  } else ind=pointMap[pt];
  return ind;
}

int PolySet::pointIndex(const Vector3d &pt)
{
  int ind;
  if(pointMap.count(pt) == 0) {
    ind=this->points.size();
    this->points.push_back(pt);
    pointMap[pt]=ind;
  } else ind=pointMap[pt];
  return ind;
}


int PolySet::append_coord(const Vector3d &coord)
{
  this->points.push_back(coord);
  return this->points.size()-1;
}

void PolySet::append_poly(const std::vector<int> &inds)
{
  this->polygons_ind.push_back(inds);	
  this->dirty = true;
}

void PolySet::append_vertex(const int ind)
{
  polygons_ind.back().push_back(ind);
  this->dirty = true;
}

void PolySet::insert_vertex(int ind)
{
  polygons_ind.back().insert(polygons_ind.back().begin(), ind);
  this->dirty = true;
}

BoundingBox PolySet::getBoundingBox() const
{
  if (this->dirty) {
    this->bbox.setNull();
    for (const auto& poly : polygons_ind) {
      for (const auto& p : poly) {
        this->bbox.extend(this->points[p]);
      }
    }
    this->dirty = false;
  }
  return this->bbox;
}

size_t PolySet::memsize() const
{
  size_t mem = 0;
  for (const auto& p : this->polygons_ind) mem += p.size() * sizeof(int);
  for (const auto& p : this->points) mem += p.size() * sizeof(Vector3d);
  mem += this->polygon.memsize() - sizeof(this->polygon);
  mem += sizeof(PolySet);
  return mem;
}

void PolySet::append(const PolySet& ps)
{
  for(auto pol: ps.polygons_ind)
  {
	  this->append_poly(pol.size());
	  for(auto ind: pol)
		  this->append_vertex(this->pointIndex(ps.points[ind]));
  }	  
  if (!dirty && !this->bbox.isNull()) {
    this->bbox.extend(ps.getBoundingBox());
  }
  if (convex) convex = unknown;
}

void PolySet::transform(const Transform3d& mat)
{
  // If mirroring transform, flip faces to avoid the object to end up being inside-out
  bool mirrored = mat.matrix().determinant() < 0;

  for (auto& v : this->points) 
      v = mat * v;

  for (auto& p : this->polygons_ind) {
    if (mirrored) std::reverse(p.begin(), p.end());
  }
  this->dirty = true;
}

bool PolySet::is_convex() const {
  if (convex || this->isEmpty()) return true;
  if (!convex) return false;
  return PolySetUtils::is_approximately_convex(*this);
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
void PolySet::quantizeVertices(std::vector<Vector3d> *pPointsOut) // TODO is this needed ?
{
  Grid3d<unsigned int> grid(GRID_FINE);
  std::vector<unsigned int> indices; // Vertex indices in one polygon
  for (auto iter = this->polygons_ind.begin(); iter != this->polygons_ind.end();) {
    IndexedFace& p = *iter;
    indices.resize(p.size());
    // Quantize all vertices. Build index list
    for (unsigned int i = 0; i < p.size(); ++i) {
      indices[i] = grid.align(this->points[p[i]]);
      if (pPointsOut && pPointsOut->size() < grid.db.size()) {
        pPointsOut->push_back(this->points[p[i]]);
      }
    }
    // Remove consecutive duplicate vertices
    auto currp = p.begin();
    for (unsigned int i = 0; i < indices.size(); ++i) {
      if (indices[i] != indices[(i + 1) % indices.size()]) {
        (*currp++) = p[i];
      }
    }
    p.erase(currp, p.end());
    if (p.size() < 3) {
      PRINTD("Removing collapsed polygon due to quantizing");
      this->polygons_ind.erase(iter);
    } else {
      iter++;
    }
  }
}


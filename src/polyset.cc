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

#include "polyset.h"
#include "polyset-utils.h"
#include "linalg.h"
#include "printutils.h"
#include "grid.h"
#include <Eigen/LU>

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

PolySet::PolySet(const Polygon2d &origin) : polygon(origin), dim(2), convex(unknown), dirty(false)
{
}

PolySet::~PolySet()
{
}

std::string PolySet::dump() const
{
	std::stringstream out;
	out << "PolySet:"
	  << "\n dimensions:" << this->dim
	  << "\n convexity:" << this->convexity
	  << "\n num polygons: " << polygons.size()
			<< "\n num outlines: " << polygon.outlines().size()
	  << "\n polygons data:";
	for (size_t i = 0; i < polygons.size(); i++) {
		out << "\n  polygon begin:";
		const Polygon *poly = &polygons[i];
		for (size_t j = 0; j < poly->size(); j++) {
			Vector3d v = poly->at(j);
			out << "\n   vertex:" << v.transpose();
		}
	}
	out << "\n outlines data:";
	out << polygon.dump();
	out << "\nPolySet end";
	return out.str();
}

void PolySet::append_poly()
{
	polygons.push_back(Polygon());
}

void PolySet::append_poly(const Polygon &poly)
{
	polygons.push_back(poly);
	this->dirty = true;
}

void PolySet::append_vertex(double x, double y, double z)
{
	append_vertex(Vector3d(x, y, z));
}

void PolySet::append_vertex(const Vector3d &v)
{
	polygons.back().push_back(v);
	this->dirty = true;
}

void PolySet::append_vertex(const Vector3f &v)
{
	append_vertex((const Vector3d &)v.cast<double>());
}

void PolySet::insert_vertex(double x, double y, double z)
{
	insert_vertex(Vector3d(x, y, z));
}

void PolySet::insert_vertex(const Vector3d &v)
{
	polygons.back().insert(polygons.back().begin(), v);
	this->dirty = true;
}

void PolySet::insert_vertex(const Vector3f &v)
{
	insert_vertex((const Vector3d &)v.cast<double>());
}

BoundingBox PolySet::getBoundingBox() const
{
	if (this->dirty) {
		this->bbox.setNull();
		for(const auto &poly : polygons) {
			for(const auto &p : poly) {
				this->bbox.extend(p);
			}
		}
		this->dirty = false;
	}
	return this->bbox;
}

size_t PolySet::memsize() const
{
	size_t mem = 0;
	for(const auto &p : this->polygons) mem += p.size() * sizeof(Vector3d);
	mem += this->polygon.memsize() - sizeof(this->polygon);
	mem += sizeof(PolySet);
	return mem;
}

void PolySet::append(const PolySet &ps)
{
	this->polygons.insert(this->polygons.end(), ps.polygons.begin(), ps.polygons.end());
	if (!dirty && !this->bbox.isNull()) {
		this->bbox.extend(ps.getBoundingBox());
	}
}

void PolySet::transform(const Transform3d &mat)
{
	// If mirroring transform, flip faces to avoid the object to end up being inside-out
	bool mirrored = mat.matrix().determinant() < 0;

	for(auto &p : this->polygons){
		for(auto &v : p) {
			v = mat * v;
		}
		if (mirrored) std::reverse(p.begin(), p.end());
	}
	this->dirty = true;
}

bool PolySet::is_convex() const {
	if (convex || this->isEmpty()) return true;
	if (!convex) return false;
	return PolysetUtils::is_approximately_convex(*this);
}

void PolySet::resize(const Vector3d &newsize, const Eigen::Matrix<bool,3,1> &autosize)
{
	BoundingBox bbox = this->getBoundingBox();

  // Find largest dimension
	int maxdim = 0;
	for (int i=1;i<3;i++) if (newsize[i] > newsize[maxdim]) maxdim = i;

	// Default scale (scale with 1 if the new size is 0)
	Vector3d scale(1,1,1);
	for (int i=0;i<3;i++) if (newsize[i] > 0) scale[i] = newsize[i] / bbox.sizes()[i];

  // Autoscale where applicable 
	double autoscale = scale[maxdim];
	Vector3d newscale;
	for (int i=0;i<3;i++) newscale[i] = !autosize[i] || (newsize[i] > 0) ? scale[i] : autoscale;
	
	Transform3d t;
	t.matrix() << 
    newscale[0], 0, 0, 0,
    0, newscale[1], 0, 0,
    0, 0, newscale[2], 0,
    0, 0, 0, 1;

	this->transform(t);
}

/*!
	Quantizes vertices by gridding them as well as merges close vertices belonging to
	neighboring grids.
	May reduce the number of polygons if polygons collapse into < 3 vertices.
*/
void PolySet::quantizeVertices()
{
	Grid3d<int> grid(GRID_FINE);
	std::vector<int> indices; // Vertex indices in one polygon
	for (std::vector<Polygon>::iterator iter = this->polygons.begin(); iter != this->polygons.end();) {
		Polygon &p = *iter;
		indices.resize(p.size());
		// Quantize all vertices. Build index list
		for (unsigned int i=0;i<p.size();i++) indices[i] = grid.align(p[i]);
		// Remove consequtive duplicate vertices
		Polygon::iterator currp = p.begin();
		for (unsigned int i=0;i<indices.size();i++) {
			if (indices[i] != indices[(i+1)%indices.size()]) {
				(*currp++) = p[i];
			}
		}
		p.erase(currp, p.end());
		if (p.size() < 3) {
			PRINTD("Removing collapsed polygon due to quantizing");
			this->polygons.erase(iter);
		}
		else {
			iter++;
		}
	}
}


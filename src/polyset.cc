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
#include "linalg.h"
#include "printutils.h"
#include "grid.h"
#include <Eigen/LU>
#ifdef ENABLE_CGAL
#include "cgalutils.h"
#endif

#include <boost/range/adaptor/reversed.hpp>


/*! /class PolySet

	The PolySet class fulfils multiple tasks, partially for historical reasons.
	FIXME: It's a bit messy and is a prime target for refactoring.

	1) Store 2D and 3D polygon meshes from all origins
	2) Store 2D outlines, used for rendering edges (2D only)
	3) Rendering of polygons and edges


	PolySet must only contain convex polygons

 */

 PolySet::PolySet()
 	: num_facets(0), num_degenerate_polygons(0), num_appends(0), dim(3), convex(unknown),
 	  dirty(false), triangles_dirty(false), quantized(0), reversed(0)
 {
 }

PolySet::PolySet(unsigned int dim, boost::tribool convex)
	: num_facets(0), num_degenerate_polygons(0), num_appends(0), dim(dim), convex(convex),
	  dirty(false), triangles_dirty(false), quantized(0), reversed(0)
{
}

PolySet::PolySet(const Polygon2d &origin)
	: polygon(origin), num_facets(0), num_degenerate_polygons(0), num_appends(0), dim(2),
	  convex(unknown), dirty(false), triangles_dirty(false), quantized(0), reversed(0)
{
}

PolySet::~PolySet()
{
}

size_t PolySet::memsize() const
{
	size_t mem = 0;
	for(const auto &p : this->polygons) mem += p.size() * sizeof(Vector3d);
	mem += this->polygon.memsize() - sizeof(this->polygon);
	mem += sizeof(PolySet);
	return mem;
}

BoundingBox PolySet::getBoundingBox() const
{
	if (this->dirty) {
		this->bbox.setNull();
		for(const auto &poly : this->polygons) {
			for(const auto &p : poly) {
				this->bbox.extend(p);
			}
		}
		this->dirty = false;
	}
	return this->bbox;
}

std::string PolySet::dump() const
{
	std::ostringstream out;
	
	out << "PolySet:"
	  << "\n dimensions:" << this->dim
	  << "\n convexity:" << this->convexity
	  << "\n num polygons: " << polygons.size()
	  << "\n num facets: " << this->num_facets
	  << "\n num degenerate polygons: " << this->num_degenerate_polygons
	  << "\n num appends: " << this->num_appends
	  << "\n num outlines: " << polygon.outlines().size()
	  << "\n num quantizations: " << this->quantized
	  << "\n num reversals: " << this->reversed
	  << "\n polygons data:";
	for (const auto &p : this->polygons) {
		out << "\n  polygon begin:";
		for (const auto &v : p) {
			out << "\n   vertex:" << v.transpose();
		} 
	}
	for (const auto &p : this->indexed_polygons) {
		out << "\n indexed polygon begin:";
		for (const auto &f : p) {
			out << "\n  face begin:";
			for (const auto &i : f) {
				out << "\n index: " << i;
			}
		}
	}
	out << "\n indexed triangles:";
	for (const auto &t : this->indexed_triangles) {
		out << "\n triangle: " << t[0] << " " << t[1] << " " << t[2];
	}
	out << "\n outlines data:";
	out << polygon.dump();
	out << "\nPolySet end";
	return out.str();
}

bool PolySet::isConvex() const {
	if (convex || this->isEmpty()) return true;
	if (!convex) return false;
	
#ifdef ENABLE_CGAL
	return CGALUtils::is_approximately_convex(*this);
#else
	return false;
#endif
}

void PolySet::copyPolygons(const PolySet &ps)
{
	if (this->polygons.size()) {
		this->polygons.clear();
	}
	if (this->indexed_polygons.size()) {
		this->indexed_polygons.clear();
		this->vi_map.clear();
	}
	if (this->indexed_triangles.size()) {
		this->indexed_triangles.clear();
	}
	this->polygons.reserve(this->polygons.size());
	this->polygons.insert(this->polygons.end(), ps.polygons.begin(), ps.polygons.end());
	this->polygon = ps.polygon;
	
	this->num_facets = ps.num_facets;
	this->num_degenerate_polygons = ps.num_degenerate_polygons;
	this->num_appends = ps.num_appends;
	this->dim = ps.dim;
	this->convex = ps.convex;
	this->setConvexity(ps.getConvexity());
	
	this->bbox = ps.bbox;
	this->dirty = ps.dirty;
	this->triangles_dirty = true;
	this->quantized = ps.quantized;
	this->reversed = ps.reversed;
}

void PolySet::append_poly()
{
	this->polygons.emplace_back(Polygon());
	this->indexed_polygons.emplace_back(IndexedPolygon());
	this->append_facet();
}

void PolySet::append_poly_only()
{
	this->polygons.emplace_back(Polygon());
	this->indexed_polygons.emplace_back(IndexedPolygon());
}

void PolySet::append_poly(const Polygon &poly)
{
	if (poly.size() < 3)
		return;

	// don't allow invalid numbers
	append_poly();
	for (const auto &v : poly) {
		append_vertex(v);
	}
	close_poly();

	this->num_appends++;

	this->dirty = true;
	this->triangles_dirty = true;
}

void PolySet::append_facet()
{
	this->indexed_polygons.back().emplace_back(IndexedFace());
	this->num_facets++;
}

void PolySet::append_vertex(double x, double y, double z)
{
	append_vertex(Vector3d(x, y, z));
}

void PolySet::append_vertex(const Vector3d &v)
{
	// do not append vertex if numeric error
	if (std::isnan(v.cast<double>()[0]) || std::isinf(v.cast<double>()[0]) ||
	    std::isnan(v.cast<double>()[1]) || std::isinf(v.cast<double>()[1]) ||
	    std::isnan(v.cast<double>()[2]) || std::isinf(v.cast<double>()[2]))
		return;

	this->polygons.back().emplace_back(v);
	size_t index = this->vertexLookup(v);

	// if v is different from the back index
	if (this->indexed_polygons.back().back().empty() ||
		this->indexed_polygons.back().back().back() != index) {
		// insert index
		this->indexed_polygons.back().back().emplace_back(index);
	}

	this->dirty = true;
	this->triangles_dirty = true;
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
	// do not insert vertex if numeric error
	if (std::isnan(v.cast<double>()[0]) || std::isinf(v.cast<double>()[0]) ||
	    std::isnan(v.cast<double>()[1]) || std::isinf(v.cast<double>()[1]) ||
	    std::isnan(v.cast<double>()[2]) || std::isinf(v.cast<double>()[2]))
		return;

	this->polygons.back().emplace(this->polygons.back().begin(), v);
	size_t index = this->vertexLookup(v);

	this->indexed_polygons.back().back().emplace(this->indexed_polygons.back().back().begin(), index);

	// if v is different from the front index
	if (this->indexed_polygons.back().back().empty() ||
		this->indexed_polygons.back().back().front() != index) {
		// insert index
		this->indexed_polygons.back().back().emplace(this->indexed_polygons.back().back().begin(), index);
	}

	this->dirty = true;
	this->triangles_dirty = true;
}

void PolySet::insert_vertex(const Vector3f &v)
{
	insert_vertex((const Vector3d &)v.cast<double>());
}

void PolySet::append(const PolySet &ps)
{
	size_t ip_size = this->indexed_polygons.size();
	this->polygons.reserve(this->polygons.size() + ps.polygons.size());
	this->polygons.insert(this->polygons.end(), ps.polygons.begin(), ps.polygons.end());
	
	// keep facet structure
	this->indexed_polygons.reserve(ip_size + ps.indexed_polygons.size());
	this->indexed_polygons.insert(this->indexed_polygons.end(), ps.indexed_polygons.begin(), ps.indexed_polygons.end());
	
	// merge indexes
	std::vector<Vector3d> ps_vertices;
	ps.getVertices<Vector3d>(ps_vertices);
	
	IndexedPolygons::iterator ip_iter = this->indexed_polygons.begin()+ip_size;
	
	while(ip_iter != this->indexed_polygons.end()) {
		for (auto &f : *ip_iter) {
			for (auto &i : f) {
				// update with new index
				i = this->vertexLookup(ps_vertices[i]);
			}
		}
		ip_iter++;
	}
	
	this->num_degenerate_polygons += ps.num_degenerate_polygons;
	this->num_facets += ps.num_facets;
	this->num_appends++;
	this->num_appends += ps.num_appends;
	this->quantized += ps.quantized;
	this->reversed += ps.reversed;
	this->triangles_dirty = true;

	if (!this->dirty && !this->bbox.isNull()) {
		this->bbox.extend(ps.getBoundingBox());
	} else {
		this->dirty = true;
	}
}

void PolySet::close_poly()
{
	this->close_facet();
	this->close_poly_only();
}

void PolySet::close_poly_only()
{
	if (this->polygons.back().empty() || this->polygons.back().size() < 3) this->polygons.pop_back();
	if (this->indexed_polygons.back().empty()) this->indexed_polygons.pop_back(); // Cull empty faces
}

void PolySet::close_facet()
{
	if (!this->indexed_polygons.back().back().empty() &&
		this->indexed_polygons.back().back().front() == this->indexed_polygons.back().back().back()) {
		this->indexed_polygons.back().back().pop_back();
	}
	if (this->indexed_polygons.back().back().size() < 3) {
		this->num_degenerate_polygons++;
		this->indexed_polygons.back().pop_back(); // Cull empty triangles
	}
}

void PolySet::transform(const Transform3d &mat)
{
	// If mirroring transform, flip faces to avoid the object to end up being inside-out
	bool mirrored = mat.matrix().determinant() < 0;

	size_t vi_map_size = this->vi_map.size();
	
	this->vi_map.clear();
	this->vi_map.reserve(vi_map_size);
	this->num_degenerate_polygons = 0;

	IndexedPolygons::iterator poly_iter = this->indexed_polygons.begin();
	IndexedPolygon::iterator face_iter;

	for(auto &p : this->polygons) {
		if (p.size() < 3) this->num_degenerate_polygons++;

		bool poly_empty = this->indexed_polygons.empty() && poly_iter->empty();
		IndexedFace::iterator vert_iter;
		
		if (!poly_empty && poly_iter != this->indexed_polygons.end()) {
			face_iter = poly_iter->begin();
			if (!face_iter->empty() && face_iter != poly_iter->end()) {
				vert_iter = face_iter->begin();
			}
		}
			
		for(auto &v : p) {
			v = mat * v;
			if (!poly_empty && poly_iter != this->indexed_polygons.end() &&
				!face_iter->empty() && face_iter != poly_iter->end()) {
				if (vert_iter != face_iter->end()) {
					*vert_iter = this->vertexLookup(v);
					vert_iter++;
				} else {
					if (mirrored) {
						for (auto &f : *poly_iter) {
							std::reverse(f.begin(), f.end());
						}
					}
					face_iter++;
					if (face_iter != poly_iter->end())
						vert_iter = face_iter->begin();
				}
			}
		}
		if (mirrored) {
			std::reverse(p.begin(), p.end());
			for (auto &f : *poly_iter) {
				std::reverse(f.begin(), f.end());
			}
			if (poly_iter->size() > 1) {
				std::reverse(poly_iter->begin(), poly_iter->end());
			}
		}
		poly_iter++;
	}
	
	this->dirty = true;
	this->triangles_dirty = true;
	if (mirrored) this->reversed++;
}

void PolySet::translate(const Vector3d &translation)
{
	size_t vi_map_size = this->vi_map.size();
	
	this->vi_map.clear();
	this->vi_map.reserve(vi_map_size);
	this->num_degenerate_polygons = 0;

	IndexedPolygons::iterator poly_iter = this->indexed_polygons.begin();
	IndexedPolygon::iterator face_iter;

	for(auto &p : this->polygons) {
		if (p.size() < 3) this->num_degenerate_polygons++;
		bool poly_empty = this->indexed_polygons.empty() && poly_iter->empty();
		IndexedFace::iterator vert_iter;
		
		if (!poly_empty && poly_iter != this->indexed_polygons.end()) {
			face_iter = poly_iter->begin();
			if (!face_iter->empty() && face_iter != poly_iter->end()) {
				vert_iter = face_iter->begin();
			}
		}

		for(auto &v : p) {
			v += translation;
			if (!poly_empty && poly_iter != this->indexed_polygons.end() &&
				!face_iter->empty() && face_iter != poly_iter->end()) {
				if (vert_iter != face_iter->end()) {
					*vert_iter = this->vertexLookup(v);
					vert_iter++;
				} else {
					face_iter++;
					if (face_iter != poly_iter->end())
						vert_iter = face_iter->begin();
				}
			}
		}
		poly_iter++;
	}
	this->dirty = true;
	this->triangles_dirty = true;
}

void PolySet::reverse()
{
	for (auto &p : this->polygons) {
		std::reverse(p.begin(), p.end());
	}
	for (auto &p : this->indexed_polygons) {
		for (auto &f : p) {
			std::reverse(f.begin(), f.end());
		}
		std::reverse(p.begin(), p.end());
	}
	this->dirty = true;
	this->triangles_dirty = true;
	this->reversed++;
}

void PolySet::resize(const Vector3d &newsize, const Eigen::Matrix<bool,3,1> &autosize)
{
	BoundingBox bbox = this->getBoundingBox();

  // Find largest dimension
	int maxdim = 0;
	for (int i=1; i<3; ++i) if (newsize[i] > newsize[maxdim]) maxdim = i;

	// Default scale (scale with 1 if the new size is 0)
	Vector3d scale(1,1,1);
	for (int i=0; i<3; ++i) if (newsize[i] > 0) scale[i] = newsize[i] / bbox.sizes()[i];

  // Autoscale where applicable 
	double autoscale = scale[maxdim];
	Vector3d newscale;
	for (int i=0; i<3; ++i) newscale[i] = !autosize[i] || (newsize[i] > 0) ? scale[i] : autoscale;
	
	Transform3d t;
	t.matrix() << 
    newscale[0], 0, 0, 0,
    0, newscale[1], 0, 0,
    0, 0, newscale[2], 0,
    0, 0, 0, 1;

	this->transform(t);
}

void PolySet::reindex() {
	size_t vi_map_size = this->vi_map.size();
	this->num_degenerate_polygons = 0;
	if (!this->indexed_polygons.empty()) {
		this->indexed_polygons.clear();
		this->vi_map.clear();
	}
	
	this->indexed_polygons.reserve(this->polygons.size());
	this->vi_map.reserve(vi_map_size);

	for (const auto &p : this->polygons) {
		if (p.size() < 3) {
			this->num_degenerate_polygons++;
			continue;
		}
		
		IndexedFace currface;
		currface.reserve(p.size());
		for (const auto &v : p) {
			// Create vertex indices and remove consecutive duplicate vertices
			auto idx = this->vertexLookup(v);
			if (currface.empty() || idx != currface.back()) currface.emplace_back(idx);
		}
		if (currface.front() == currface.back()) currface.pop_back();
		if (currface.size() >= 3) {
			this->indexed_polygons.emplace_back(IndexedPolygon());
			this->indexed_polygons.back().emplace_back(currface);
		}
	}
}

/*!
	Quantizes vertices by gridding them as well as merges close vertices belonging to
	neighboring grids.
	May reduce the number of polygons if polygons collapse into < 3 vertices.
	Flattens multiple facets into single indexed polygon
*/
void PolySet::quantizeVertices()
{
	Grid3d<size_t> grid(GRID_FINE);
	size_t vi_map_size = this->vi_map.size();
	
	if (vi_map_size) {
		this->indexed_polygons.clear();
		this->vi_map.clear();
	}
	this->num_degenerate_polygons = 0;
	this->num_facets = 0;
	
	this->indexed_polygons.reserve(this->polygons.size());
	this->vi_map.reserve(this->polygons.size());
	
	for (Polygons::iterator iter = this->polygons.begin(); iter != this->polygons.end();) {
		Polygon &p = *iter;
		IndexedFace indices;
		IndexedFace face;
		indices.reserve(p.size());
		face.reserve(p.size());
		// Quantize all vertices. Build index list
		for (auto &v : p) {
			indices.emplace_back(grid.align(v));
		}
		// Remove consecutive duplicate vertices
		Polygon::iterator currp = p.begin();
		for (size_t i = 0; i < indices.size(); ++i) {
			if (indices[i] != indices[(i+1)%indices.size()]) {
				(*currp++) = p[i];
				face.emplace_back(this->vertexLookup(p[i]));
			}
		}
		p.erase(currp, p.end());

		if (p.size() < 3) {
			PRINTD("Removing collapsed polygon due to quantizing");
			this->polygons.erase(iter);
			this->num_degenerate_polygons++;
		}
		if (face.size() >= 3){
			this->indexed_polygons.emplace_back(IndexedPolygon());
			this->indexed_polygons.back().emplace_back(face);
			this->num_facets++;
			iter++;
		}
	}

	this->quantized++;
	this->triangles_dirty = true;
}

/* Tessellation of 3d PolySet faces
	 
	 This code is for tessellating the faces of a 3d PolySet, assuming that
	 the faces are near-planar polygons.
	 
	 The purpose of this code is originally to fix github issue 349. Our CGAL
	 kernel does not accept polygons for Nef_Polyhedron_3 if each of the
	 points is not exactly coplanar. "Near-planar" or "Almost planar" polygons
	 often occur due to rounding issues on, for example, polyhedron() input.
	 By tessellating the 3d polygon into individual smaller tiles that
	 are perfectly coplanar (triangles, for example), we can get CGAL to accept
	 the polyhedron() input.
*/
	
/* Given a 3D PolySet with near planar polygonal faces, tessellate the
	 faces. As of writing, our only tessellation method is triangulation
	 using CGAL's Constrained Delaunay algorithm. This code assumes the input
	 polyset has simple polygon faces with no holes.
	 The tessellation will be robust wrt. degenerate and self-intersecting
*/
void PolySet::tessellate()
{
	if (this->triangles_dirty) {
		this->indexed_triangles.clear();
		this->indexed_triangles.reserve(this->num_facets);
		this->num_facets = 0;
		this->num_degenerate_polygons = 0;
		
		if (this->indexed_polygons.empty()) {
			this->reindex();
		}
		
		std::vector<Vector3f> verts;
		this->getVertices<Vector3f>(verts);

		// use indexed polygons to rebuild triangles and polygons
		this->polygons.clear();
		this->polygons.reserve(this->indexed_polygons.size());
		
		// Tessellate indexed mesh
		for (auto &faces : this->indexed_polygons) {
			IndexedTriangles triangles;
			auto err = false;
			
			if (faces.size() == 1 && faces[0].size() == 3) {
				triangles.emplace_back(faces[0][0], faces[0][1], faces[0][2]);
			} else {
				err = GeometryUtils::tessellatePolygonWithHoles(verts, faces, triangles, nullptr, false);
			}
			if (!err) {
				for (const auto &t : triangles) {
					this->polygons.emplace_back(Polygon());
					this->polygons.back().emplace_back(verts[t[0]].cast<double>());
					this->polygons.back().emplace_back(verts[t[1]].cast<double>());
					this->polygons.back().emplace_back(verts[t[2]].cast<double>());
					this->num_facets++;
				}
				this->indexed_triangles.insert(this->indexed_triangles.end(), triangles.begin(), triangles.end());
			}
		}
		
		this->triangles_dirty = false;
	}
}

// Project all polygons (also back-facing) into a Polygon2d instance.
// It's important to select all faces, since filtering by normal vector here
// will trigger floating point incertainties and cause problems later.
Polygon2d *PolySet::project() const {
	auto poly = new Polygon2d;

	for (const auto &p : this->polygons) {
		Outline2d outline;
		for (const auto &v : p) {
			outline.vertices.emplace_back(v[0], v[1]);
		}
		poly->addOutline(outline);
	}
	return poly;
}

size_t PolySet::vertexLookup(const Vector3d &vertex) {
	VertexIndexMap::iterator entry = this->vi_map.find(vertex);
	if (entry == this->vi_map.end()) {
		std::pair<VertexIndexMap::iterator,bool> result = this->vi_map.emplace(vertex,this->vi_map.size());
		entry = result.first;
	}
	return entry->second;
}
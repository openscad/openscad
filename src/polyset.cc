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
#include <boost/foreach.hpp>

/*! /class PolySet

	The PolySet class fulfils multiple tasks, partially for historical reasons.
	FIXME: It's a bit messy and is a prime target for refactoring.

	1) Store 2D and 3D polygon meshes from all origins
	2) Store 2D outlines, used for rendering edges (2D only)
	3) Rendering of polygons and edges


	PolySet must only contain convex polygons

 */

PolySet::PolySet(unsigned int dim, boost::tribool convex) : dim(dim), convex(convex)
{
}

PolySet::PolySet(const Polygon2d &origin) : polygon(origin), dim(2), convex(unknown)
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

void PolySet::append_vertex(double x, double y, double z)
{
	append_vertex(Vector3d(x, y, z));
}

void PolySet::append_vertex(const Vector3d &v)
{
	polygons.back().push_back(v);
}

void PolySet::append_vertex(const Vector3f &v)
{
	polygons.back().push_back(v.cast<double>());
}

void PolySet::insert_vertex(double x, double y, double z)
{
	insert_vertex(Vector3d(x, y, z));
}

void PolySet::insert_vertex(const Vector3d &v)
{
	polygons.back().insert(polygons.back().begin(), v);
}

void PolySet::insert_vertex(const Vector3f &v)
{
	polygons.back().insert(polygons.back().begin(), v.cast<double>());
}

BoundingBox PolySet::getBoundingBox() const
{
	BoundingBox bbox;
	for (size_t i = 0; i < polygons.size(); i++) {
		const Polygon &poly = polygons[i];
		for (size_t j = 0; j < poly.size(); j++) {
			const Vector3d &p = poly[j];
			bbox.extend(p);
		}
	}
	return bbox;
}

size_t PolySet::memsize() const
{
	size_t mem = 0;
	BOOST_FOREACH(const Polygon &p, this->polygons) mem += p.size() * sizeof(Vector3d);
	mem += this->polygon.memsize() - sizeof(this->polygon);
	mem += sizeof(PolySet);
	return mem;
}

void PolySet::append(const PolySet &ps)
{
	this->polygons.insert(this->polygons.end(), ps.polygons.begin(), ps.polygons.end());
}

void PolySet::transform(const Transform3d &mat)
{
	// If mirroring transform, flip faces to avoid the object to end up being inside-out
	bool mirrored = mat.matrix().determinant() < 0;

	BOOST_FOREACH(Polygon &p, this->polygons) {
		BOOST_FOREACH(Vector3d &v, p) {
			v = mat * v;
		}
		if (mirrored) std::reverse(p.begin(), p.end());
	}
}

bool PolySet::is_convex() const {
	if (convex || this->isEmpty()) return true;
	if (!convex) return false;
	return PolysetUtils::is_approximately_convex(*this);
}

void PolySet::resize(Vector3d newsize, const Eigen::Matrix<bool,3,1> &autosize)
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
	int numverts = 0;
	std::vector<int> indices; // Vertex indices in one polygon
	for (std::vector<Polygon>::iterator iter = this->polygons.begin(); iter != this->polygons.end();) {
		Polygon &p = *iter;
		indices.resize(p.size());
		// Quantize all vertices. Build index list
		for (int i=0;i<p.size();i++) indices[i] = grid.align(p[i]);
		// Remove consequtive duplicate vertices
		Polygon::iterator currp = p.begin();
		for (int i=0;i<indices.size();i++) {
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

// all GL functions grouped together here
#ifndef NULLGL
static void gl_draw_triangle(GLint *shaderinfo, const Vector3d &p0, const Vector3d &p1, const Vector3d &p2, bool e0, bool e1, bool e2, double z, bool mirrored)
{
	double ax = p1[0] - p0[0], bx = p1[0] - p2[0];
	double ay = p1[1] - p0[1], by = p1[1] - p2[1];
	double az = p1[2] - p0[2], bz = p1[2] - p2[2];
	double nx = ay*bz - az*by;
	double ny = az*bx - ax*bz;
	double nz = ax*by - ay*bx;
	double nl = sqrt(nx*nx + ny*ny + nz*nz);
	glNormal3d(nx / nl, ny / nl, nz / nl);
#ifdef ENABLE_OPENCSG
	if (shaderinfo) {
		double e0f = e0 ? 2.0 : -1.0;
		double e1f = e1 ? 2.0 : -1.0;
		double e2f = e2 ? 2.0 : -1.0;
		glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
		glVertexAttrib3d(shaderinfo[4], p1[0], p1[1], p1[2] + z);
		glVertexAttrib3d(shaderinfo[5], p2[0], p2[1], p2[2] + z);
		glVertexAttrib3d(shaderinfo[6], 0.0, 1.0, 0.0);
		glVertex3d(p0[0], p0[1], p0[2] + z);
		if (!mirrored) {
			glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
			glVertexAttrib3d(shaderinfo[4], p0[0], p0[1], p0[2] + z);
			glVertexAttrib3d(shaderinfo[5], p2[0], p2[1], p2[2] + z);
			glVertexAttrib3d(shaderinfo[6], 0.0, 0.0, 1.0);
			glVertex3d(p1[0], p1[1], p1[2] + z);
		}
		glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
		glVertexAttrib3d(shaderinfo[4], p0[0], p0[1], p0[2] + z);
		glVertexAttrib3d(shaderinfo[5], p1[0], p1[1], p1[2] + z);
		glVertexAttrib3d(shaderinfo[6], 1.0, 0.0, 0.0);
		glVertex3d(p2[0], p2[1], p2[2] + z);
		if (mirrored) {
			glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
			glVertexAttrib3d(shaderinfo[4], p0[0], p0[1], p0[2] + z);
			glVertexAttrib3d(shaderinfo[5], p2[0], p2[1], p2[2] + z);
			glVertexAttrib3d(shaderinfo[6], 0.0, 0.0, 1.0);
			glVertex3d(p1[0], p1[1], p1[2] + z);
		}
	}
	else
#endif
	{
		glVertex3d(p0[0], p0[1], p0[2] + z);
		if (!mirrored)
			glVertex3d(p1[0], p1[1], p1[2] + z);
		glVertex3d(p2[0], p2[1], p2[2] + z);
		if (mirrored)
			glVertex3d(p1[0], p1[1], p1[2] + z);
	}
}

void PolySet::render_surface(Renderer::csgmode_e csgmode, const Transform3d &m, GLint *shaderinfo) const
{
	PRINTD("Polyset render");
	bool mirrored = m.matrix().determinant() < 0;
#ifdef ENABLE_OPENCSG
	if (shaderinfo) {
		glUniform1f(shaderinfo[7], shaderinfo[9]);
		glUniform1f(shaderinfo[8], shaderinfo[10]);
	}
#endif /* ENABLE_OPENCSG */
	if (this->dim == 2) {
		// Render 2D objects 1mm thick, but differences slightly larger
		double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0);
		glBegin(GL_TRIANGLES);

		// Render top+bottom
		for (double z = -zbase/2; z < zbase; z += zbase) {
			for (size_t i = 0; i < polygons.size(); i++) {
				const Polygon *poly = &polygons[i];
				if (poly->size() == 3) {
					if (z < 0) {
						gl_draw_triangle(shaderinfo, poly->at(0), poly->at(2), poly->at(1), true, true, true, z, mirrored);
					} else {
						gl_draw_triangle(shaderinfo, poly->at(0), poly->at(1), poly->at(2), true, true, true, z, mirrored);
					}
				}
				else if (poly->size() == 4) {
					if (z < 0) {
						gl_draw_triangle(shaderinfo, poly->at(0), poly->at(3), poly->at(1), true, false, true, z, mirrored);
						gl_draw_triangle(shaderinfo, poly->at(2), poly->at(1), poly->at(3), true, false, true, z, mirrored);
					} else {
						gl_draw_triangle(shaderinfo, poly->at(0), poly->at(1), poly->at(3), true, false, true, z, mirrored);
						gl_draw_triangle(shaderinfo, poly->at(2), poly->at(3), poly->at(1), true, false, true, z, mirrored);
					}
				}
				else {
					Vector3d center = Vector3d::Zero();
					for (size_t j = 0; j < poly->size(); j++) {
						center[0] += poly->at(j)[0];
						center[1] += poly->at(j)[1];
					}
					center[0] /= poly->size();
					center[1] /= poly->size();
					for (size_t j = 1; j <= poly->size(); j++) {
						if (z < 0) {
							gl_draw_triangle(shaderinfo, center, poly->at(j % poly->size()), poly->at(j - 1),
									false, true, false, z, mirrored);
						} else {
							gl_draw_triangle(shaderinfo, center, poly->at(j - 1), poly->at(j % poly->size()),
									false, true, false, z, mirrored);
						}
					}
				}
			}
		}

		// Render sides
		if (polygon.outlines().size() > 0) {
			BOOST_FOREACH(const Outline2d &o, polygon.outlines()) {
				for (size_t j = 1; j <= o.vertices.size(); j++) {
					Vector3d p1(o.vertices[j-1][0], o.vertices[j-1][1], -zbase/2);
					Vector3d p2(o.vertices[j-1][0], o.vertices[j-1][1], zbase/2);
					Vector3d p3(o.vertices[j % o.vertices.size()][0], o.vertices[j % o.vertices.size()][1], -zbase/2);
					Vector3d p4(o.vertices[j % o.vertices.size()][0], o.vertices[j % o.vertices.size()][1], zbase/2);
					gl_draw_triangle(shaderinfo, p2, p1, p3, true, true, false, 0, mirrored);
					gl_draw_triangle(shaderinfo, p2, p3, p4, false, true, true, 0, mirrored);
				}
			}
		}
		else {
			// If we don't have borders, use the polygons as borders.
			// FIXME: When is this used?
			const Polygons *borders_p = &polygons;
			for (size_t i = 0; i < borders_p->size(); i++) {
				const Polygon *poly = &borders_p->at(i);
				for (size_t j = 1; j <= poly->size(); j++) {
					Vector3d p1 = poly->at(j - 1), p2 = poly->at(j - 1);
					Vector3d p3 = poly->at(j % poly->size()), p4 = poly->at(j % poly->size());
					p1[2] -= zbase/2, p2[2] += zbase/2;
					p3[2] -= zbase/2, p4[2] += zbase/2;
					gl_draw_triangle(shaderinfo, p2, p1, p3, true, true, false, 0, mirrored);
					gl_draw_triangle(shaderinfo, p2, p3, p4, false, true, true, 0, mirrored);
				}
			}
		}
		glEnd();
	} else if (this->dim == 3) {
		for (size_t i = 0; i < polygons.size(); i++) {
			const Polygon *poly = &polygons[i];
			glBegin(GL_TRIANGLES);
			if (poly->size() == 3) {
				gl_draw_triangle(shaderinfo, poly->at(0), poly->at(1), poly->at(2), true, true, true, 0, mirrored);
			}
			else if (poly->size() == 4) {
				gl_draw_triangle(shaderinfo, poly->at(0), poly->at(1), poly->at(3), true, false, true, 0, mirrored);
				gl_draw_triangle(shaderinfo, poly->at(2), poly->at(3), poly->at(1), true, false, true, 0, mirrored);
			}
			else {
				Vector3d center = Vector3d::Zero();
				for (size_t j = 0; j < poly->size(); j++) {
					center[0] += poly->at(j)[0];
					center[1] += poly->at(j)[1];
					center[2] += poly->at(j)[2];
				}
				center[0] /= poly->size();
				center[1] /= poly->size();
				center[2] /= poly->size();
				for (size_t j = 1; j <= poly->size(); j++) {
					gl_draw_triangle(shaderinfo, center, poly->at(j - 1), poly->at(j % poly->size()), false, true, false, 0, mirrored);
				}
			}
			glEnd();
		}
	}
	else {
		assert(false && "Cannot render object with no dimension");
	}
}

/*! This is used in throwntogether and CGAL mode

	csgmode is set to CSGMODE_NONE in CGAL mode. In this mode a pure 2D rendering is performed.

	For some reason, this is not used to render edges in Preview mode
*/
void PolySet::render_edges(Renderer::csgmode_e csgmode) const
{
	glDisable(GL_LIGHTING);
	if (this->dim == 2) {
		if (csgmode == Renderer::CSGMODE_NONE) {
			// Render only outlines
			BOOST_FOREACH(const Outline2d &o, polygon.outlines()) {
				glBegin(GL_LINE_LOOP);
				BOOST_FOREACH(const Vector2d &v, o.vertices) {
					glVertex3d(v[0], v[1], -0.1);
				}
				glEnd();
			}
		}
		else {
			// Render 2D objects 1mm thick, but differences slightly larger
			double zbase = 1 + ((csgmode & CSGMODE_DIFFERENCE_FLAG) ? 0.1 : 0);

			BOOST_FOREACH(const Outline2d &o, polygon.outlines()) {
				// Render top+bottom outlines
				for (double z = -zbase/2; z < zbase; z += zbase) {
					glBegin(GL_LINE_LOOP);
					BOOST_FOREACH(const Vector2d &v, o.vertices) {
						glVertex3d(v[0], v[1], z);
					}
					glEnd();
				}
				// Render sides
				glBegin(GL_LINES);
				BOOST_FOREACH(const Vector2d &v, o.vertices) {
					glVertex3d(v[0], v[1], -zbase/2);
					glVertex3d(v[0], v[1], +zbase/2);
				}
				glEnd();
			}
		}
	} else if (dim == 3) {
		for (size_t i = 0; i < polygons.size(); i++) {
			const Polygon *poly = &polygons[i];
			glBegin(GL_LINE_LOOP);
			for (size_t j = 0; j < poly->size(); j++) {
				const Vector3d &p = poly->at(j);
				glVertex3d(p[0], p[1], p[2]);
			}
			glEnd();
		}
	}
	else {
		assert(false && "Cannot render object with no dimension");
	}
	glEnable(GL_LIGHTING);
}

#else //NULLGL
static void gl_draw_triangle(GLint *shaderinfo, const Vector3d &p0, const Vector3d &p1, const Vector3d &p2, bool e0, bool e1, bool e2, double z, bool mirrored) {}
void PolySet::render_surface(Renderer::csgmode_e csgmode, const Transform3d &m, GLint *shaderinfo) const {}
void PolySet::render_edges(Renderer::csgmode_e csgmode) const {}
#endif //NULLGL

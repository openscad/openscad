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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "polyset.h"
#include "linalg.h"
#include <Eigen/LU>
#include <boost/foreach.hpp>
#include "polyset1.cc"
/*! /class PolySet

	The PolySet class fulfils multiple tasks, partially for historical reasons.
	FIXME: It's a bit messy and is a prime target for refactoring.

	1) Store 2D and 3D polygon meshes from all origins
	2) Store 2D outlines, used for rendering edges
	3) Rendering of polygons and edges

 */

PolySet::PolySet() : grid(GRID_FINE), is2d(false), convexity(1)
{
}

PolySet::~PolySet()
{
}

std::string PolySet::dump() const
{
	std::stringstream out;
	out << "PolySet:"
	  << "\n dimensions:" << std::string( this->is2d ? "2" : "3" )
	  << "\n convexity:" << this->convexity
	  << "\n num polygons: " << polygons.size()
	  << "\n num borders: " << borders.size()
	  << "\n polygons data:";
	for (size_t i = 0; i < polygons.size(); i++) {
		out << "\n  polygon begin:";
		const Polygon *poly = &polygons[i];
		for (size_t j = 0; j < poly->size(); j++) {
			Vector3d v = poly->at(j);
			out << "\n   vertex:" << v.transpose();
		}
	}
	out << "\n borders data:";
	for (size_t i = 0; i < borders.size(); i++) {
		out << "\n  border polygon begin:";
		const Polygon *poly = &borders[i];
		for (size_t j = 0; j < poly->size(); j++) {
			Vector3d v = poly->at(j);
			out << "\n   vertex:" << v.transpose();
		}
	}
	out << "\nPolySet end";
	return out.str();
}

void PolySet::append_poly()
{
	polygons.push_back(Polygon());
}

void PolySet::append_vertex(double x, double y, double z)
{
	grid.align(x, y, z);
	polygons.back().push_back(Vector3d(x, y, z));
}

void PolySet::insert_vertex(double x, double y, double z)
{
	grid.align(x, y, z);
	polygons.back().insert(polygons.back().begin(), Vector3d(x, y, z));
}

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

		draw_triangle(shaderinfo, p0, p1, p2, e0f, e1f, e2f, z, mirrored);
	}
	else
#endif
	{

		draw_tri(p0, p1, p2, mirrored);
		
	}
}

void PolySet::render_surface(csgmode_e csgmode, const Transform3d &m, GLint *shaderinfo) const
{
	bool mirrored = m.matrix().determinant() < 0;
#ifdef ENABLE_OPENCSG
	if (shaderinfo) {
		glUniform1f(shaderinfo[7], shaderinfo[9]);
		glUniform1f(shaderinfo[8], shaderinfo[10]);
	}
#endif /* ENABLE_OPENCSG */
	if (this->is2d) {
		// Render 2D objects 1mm thick, but differences slightly larger
		double zbase = 1 + (csgmode & CSGMODE_DIFFERENCE_FLAG) * 0.1;
		glBegin(GL_TRIANGLES);
		for (double z = -zbase/2; z < zbase; z += zbase)
		{
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
		const std::vector<Polygon> *borders_p = &borders;
		if (borders_p->size() == 0)
			borders_p = &polygons;
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
		glEnd();
	} else {
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
}

void PolySet::render_edges(csgmode_e csgmode) const
{
	glDisable(GL_LIGHTING);
	if (this->is2d) {
		// Render 2D objects 1mm thick, but differences slightly larger
		double zbase = 1 + (csgmode & CSGMODE_DIFFERENCE_FLAG) * 0.1;
		for (double z = -zbase/2; z < zbase; z += zbase)
		{
			for (size_t i = 0; i < borders.size(); i++) {
				const Polygon *poly = &borders[i];
				glBegin(GL_LINE_LOOP);
				for (size_t j = 0; j < poly->size(); j++) {
					const Vector3d &p = poly->at(j);
					glVertex3d(p[0], p[1], z);
				}
				glEnd();
			}
		}
		for (size_t i = 0; i < borders.size(); i++) {
			const Polygon *poly = &borders[i];
			glBegin(GL_LINES);
			for (size_t j = 0; j < poly->size(); j++) {
				const Vector3d &p = poly->at(j);
				glVertex3d(p[0], p[1], -zbase/2);
				glVertex3d(p[0], p[1], +zbase/2);
			}
			glEnd();
		}
	} else {
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
	glEnable(GL_LIGHTING);
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
	BOOST_FOREACH(const Polygon &p, this->borders) mem += p.size() * sizeof(Vector3d);
	mem += this->grid.db.size() * (3 * sizeof(int64_t) + sizeof(void*)) + sizeof(Grid3d<void*>);
	mem += sizeof(PolySet);
	return mem;
}

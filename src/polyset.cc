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
#include "printutils.h"
#include "Preferences.h"
#ifdef ENABLE_CGAL
#include <CGAL/assertions_behaviour.h>
#include <CGAL/exceptions.h>
#endif
#include <Eigen/Core>
#include <Eigen/LU>

QCache<QString,PolySet::ps_cache_entry> PolySet::ps_cache(100);

PolySet::ps_cache_entry::ps_cache_entry(PolySet *ps) :
		ps(ps), msg(print_messages_stack.last()) { }

PolySet::ps_cache_entry::~ps_cache_entry() {
	ps->unlink();
}

PolySet::PolySet() : grid(GRID_FINE)
{
	is2d = false;
	convexity = 1;
	refcount = 1;
}

PolySet::~PolySet()
{
	assert(refcount == 0);
}

PolySet* PolySet::link()
{
	refcount++;
	return this;
}

void PolySet::unlink()
{
	if (--refcount == 0)
		delete this;
}

void PolySet::append_poly()
{
	polygons.append(Polygon());
}

void PolySet::append_vertex(double x, double y, double z)
{
	grid.align(x, y, z);
	polygons.last().append(Point(x, y, z));
}

void PolySet::insert_vertex(double x, double y, double z)
{
	grid.align(x, y, z);
	polygons.last().insert(0, Point(x, y, z));
}

static void gl_draw_triangle(GLint *shaderinfo, const PolySet::Point *p0, const PolySet::Point *p1, const PolySet::Point *p2, bool e0, bool e1, bool e2, double z, bool mirrored)
{
	double ax = p1->x - p0->x, bx = p1->x - p2->x;
	double ay = p1->y - p0->y, by = p1->y - p2->y;
	double az = p1->z - p0->z, bz = p1->z - p2->z;
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
		glVertexAttrib3d(shaderinfo[4], p1->x, p1->y, p1->z + z);
		glVertexAttrib3d(shaderinfo[5], p2->x, p2->y, p2->z + z);
		glVertexAttrib3d(shaderinfo[6], 0.0, 1.0, 0.0);
		glVertex3d(p0->x, p0->y, p0->z + z);
		if (!mirrored) {
			glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
			glVertexAttrib3d(shaderinfo[4], p0->x, p0->y, p0->z + z);
			glVertexAttrib3d(shaderinfo[5], p2->x, p2->y, p2->z + z);
			glVertexAttrib3d(shaderinfo[6], 0.0, 0.0, 1.0);
			glVertex3d(p1->x, p1->y, p1->z + z);
		}
		glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
		glVertexAttrib3d(shaderinfo[4], p0->x, p0->y, p0->z + z);
		glVertexAttrib3d(shaderinfo[5], p1->x, p1->y, p1->z + z);
		glVertexAttrib3d(shaderinfo[6], 1.0, 0.0, 0.0);
		glVertex3d(p2->x, p2->y, p2->z + z);
		if (mirrored) {
			glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
			glVertexAttrib3d(shaderinfo[4], p0->x, p0->y, p0->z + z);
			glVertexAttrib3d(shaderinfo[5], p2->x, p2->y, p2->z + z);
			glVertexAttrib3d(shaderinfo[6], 0.0, 0.0, 1.0);
			glVertex3d(p1->x, p1->y, p1->z + z);
		}
	}
	else
#endif
	{
		glVertex3d(p0->x, p0->y, p0->z + z);
		if (!mirrored)
			glVertex3d(p1->x, p1->y, p1->z + z);
		glVertex3d(p2->x, p2->y, p2->z + z);
		if (mirrored)
			glVertex3d(p1->x, p1->y, p1->z + z);
	}
}

void PolySet::render_surface(colormode_e colormode, csgmode_e csgmode, double *m, GLint *shaderinfo) const
{
	Eigen::Matrix3f m3f;
	m3f << m[0], m[4], m[8],
		m[1], m[5], m[9],
		m[2], m[6], m[10];
	bool mirrored = m3f.determinant() < 0;

	if (colormode == COLORMODE_MATERIAL) {
		const QColor &col = Preferences::inst()->color(Preferences::OPENCSG_FACE_FRONT_COLOR);
		glColor3f(col.redF(), col.greenF(), col.blueF());
#ifdef ENABLE_OPENCSG
		if (shaderinfo) {
			glUniform4f(shaderinfo[1], col.redF(), col.greenF(), col.blueF(), 1.0f);
			glUniform4f(shaderinfo[2], 255 / 255.0f, 236 / 255.0f, 94 / 255.0f, 1.0f);
		}
#endif /* ENABLE_OPENCSG */
	}
	if (colormode == COLORMODE_CUTOUT) {
		const QColor &col = Preferences::inst()->color(Preferences::OPENCSG_FACE_BACK_COLOR);
		glColor3f(col.redF(), col.greenF(), col.blueF());
#ifdef ENABLE_OPENCSG
		if (shaderinfo) {
			glUniform4f(shaderinfo[1], 157 / 255.0f, 203 / 255.0f, 81 / 255.0f, 1.0f);
			glUniform4f(shaderinfo[2], 171 / 255.0f, 216 / 255.0f, 86 / 255.0f, 1.0f);
		}
#endif /* ENABLE_OPENCSG */
	}
	if (colormode == COLORMODE_HIGHLIGHT) {
		glColor4ub(255, 157, 81, 128);
#ifdef ENABLE_OPENCSG
		if (shaderinfo) {
			glUniform4f(shaderinfo[1], 255 / 255.0f, 157 / 255.0f, 81 / 255.0f, 0.5f);
			glUniform4f(shaderinfo[2], 255 / 255.0f, 171 / 255.0f, 86 / 255.0f, 0.5f);
		}
#endif /* ENABLE_OPENCSG */
	}
	if (colormode == COLORMODE_BACKGROUND) {
		glColor4ub(180, 180, 180, 128);
#ifdef ENABLE_OPENCSG
		if (shaderinfo) {
			glUniform4f(shaderinfo[1], 180 / 255.0f, 180 / 255.0f, 180 / 255.0f, 0.5f);
			glUniform4f(shaderinfo[2], 150 / 255.0f, 150 / 255.0f, 150 / 255.0f, 0.5f);
		}
#endif /* ENABLE_OPENCSG */
	}
#ifdef ENABLE_OPENCSG
	if (shaderinfo) {
		glUniform1f(shaderinfo[7], shaderinfo[9]);
		glUniform1f(shaderinfo[8], shaderinfo[10]);
	}
#endif /* ENABLE_OPENCSG */
	if (this->is2d) {
		double zbase = csgmode;
		glBegin(GL_TRIANGLES);
		for (double z = -zbase/2; z < zbase; z += zbase)
		{
			for (int i = 0; i < polygons.size(); i++) {
				const Polygon *poly = &polygons[i];
				if (poly->size() == 3) {
					if (z < 0) {
						gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(2), &poly->at(1), true, true, true, z, mirrored);
					} else {
						gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(1), &poly->at(2), true, true, true, z, mirrored);
					}
				}
				else if (poly->size() == 4) {
					if (z < 0) {
						gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(3), &poly->at(1), true, false, true, z, mirrored);
						gl_draw_triangle(shaderinfo, &poly->at(2), &poly->at(1), &poly->at(3), true, false, true, z, mirrored);
					} else {
						gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(1), &poly->at(3), true, false, true, z, mirrored);
						gl_draw_triangle(shaderinfo, &poly->at(2), &poly->at(3), &poly->at(1), true, false, true, z, mirrored);
					}
				}
				else {
					Point center;
					for (int j = 0; j < poly->size(); j++) {
						center.x += poly->at(j).x;
						center.y += poly->at(j).y;
					}
					center.x /= poly->size();
					center.y /= poly->size();
					for (int j = 1; j <= poly->size(); j++) {
						if (z < 0) {
							gl_draw_triangle(shaderinfo, &center, &poly->at(j % poly->size()), &poly->at(j - 1),
									false, true, false, z, mirrored);
						} else {
							gl_draw_triangle(shaderinfo, &center, &poly->at(j - 1), &poly->at(j % poly->size()),
									false, true, false, z, mirrored);
						}
					}
				}
			}
		}
		const QVector<Polygon> *borders_p = &borders;
		if (borders_p->size() == 0)
			borders_p = &polygons;
		for (int i = 0; i < borders_p->size(); i++) {
			const Polygon *poly = &borders_p->at(i);
			for (int j = 1; j <= poly->size(); j++) {
				Point p1 = poly->at(j - 1), p2 = poly->at(j - 1);
				Point p3 = poly->at(j % poly->size()), p4 = poly->at(j % poly->size());
				p1.z -= zbase/2, p2.z += zbase/2;
				p3.z -= zbase/2, p4.z += zbase/2;
				gl_draw_triangle(shaderinfo, &p2, &p1, &p3, true, true, false, 0, mirrored);
				gl_draw_triangle(shaderinfo, &p2, &p3, &p4, false, true, true, 0, mirrored);
			}
		}
		glEnd();
	} else {
		for (int i = 0; i < polygons.size(); i++) {
			const Polygon *poly = &polygons[i];
			glBegin(GL_TRIANGLES);
			if (poly->size() == 3) {
				gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(1), &poly->at(2), true, true, true, 0, mirrored);
			}
			else if (poly->size() == 4) {
				gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(1), &poly->at(3), true, false, true, 0, mirrored);
				gl_draw_triangle(shaderinfo, &poly->at(2), &poly->at(3), &poly->at(1), true, false, true, 0, mirrored);
			}
			else {
				Point center;
				for (int j = 0; j < poly->size(); j++) {
					center.x += poly->at(j).x;
					center.y += poly->at(j).y;
					center.z += poly->at(j).z;
				}
				center.x /= poly->size();
				center.y /= poly->size();
				center.z /= poly->size();
				for (int j = 1; j <= poly->size(); j++) {
					gl_draw_triangle(shaderinfo, &center, &poly->at(j - 1), &poly->at(j % poly->size()), false, true, false, 0, mirrored);
				}
			}
			glEnd();
		}
	}
}

void PolySet::render_edges(colormode_e colormode, csgmode_e csgmode) const
{
	if (colormode == COLORMODE_MATERIAL)
		glColor3ub(255, 236, 94);
	if (colormode == COLORMODE_CUTOUT)
		glColor3ub(171, 216, 86);
	if (colormode == COLORMODE_HIGHLIGHT)
		glColor4ub(255, 171, 86, 128);
	if (colormode == COLORMODE_BACKGROUND)
		glColor4ub(150, 150, 150, 128);
	if (this->is2d) {
		double zbase = csgmode;
		for (double z = -zbase/2; z < zbase; z += zbase)
		{
			for (int i = 0; i < borders.size(); i++) {
				const Polygon *poly = &borders[i];
				glBegin(GL_LINE_LOOP);
				for (int j = 0; j < poly->size(); j++) {
					const Point *p = &poly->at(j);
					glVertex3d(p->x, p->y, z);
				}
				glEnd();
			}
		}
		for (int i = 0; i < borders.size(); i++) {
			const Polygon *poly = &borders[i];
			glBegin(GL_LINES);
			for (int j = 0; j < poly->size(); j++) {
				const Point *p = &poly->at(j);
				glVertex3d(p->x, p->y, -zbase/2);
				glVertex3d(p->x, p->y, +zbase/2);
			}
			glEnd();
		}
	} else {
		for (int i = 0; i < polygons.size(); i++) {
			const Polygon *poly = &polygons[i];
			glBegin(GL_LINE_LOOP);
			for (int j = 0; j < poly->size(); j++) {
				const Point *p = &poly->at(j);
				glVertex3d(p->x, p->y, p->z);
			}
			glEnd();
		}
	}
}

#ifdef ENABLE_CGAL

#undef GEN_SURFACE_DEBUG

class CGAL_Build_PolySet : public CGAL::Modifier_base<CGAL_HDS>
{
public:
	typedef CGAL_HDS::Vertex::Point Point;

	const PolySet *ps;
	CGAL_Build_PolySet(const PolySet *ps) : ps(ps) { }

	void operator()(CGAL_HDS& hds)
	{
		CGAL_Polybuilder B(hds, true);

		QList<PolySet::Point> vertices;
		Grid3d<int> vertices_idx(GRID_FINE);

		for (int i = 0; i < ps->polygons.size(); i++) {
			const PolySet::Polygon *poly = &ps->polygons[i];
			for (int j = 0; j < poly->size(); j++) {
				const PolySet::Point *p = &poly->at(j);
				if (!vertices_idx.has(p->x, p->y, p->z)) {
					vertices_idx.data(p->x, p->y, p->z) = vertices.size();
					vertices.append(*p);
				}
			}
		}

		B.begin_surface(vertices.size(), ps->polygons.size());
#ifdef GEN_SURFACE_DEBUG
		printf("=== CGAL Surface ===\n");
#endif

		for (int i = 0; i < vertices.size(); i++) {
			const PolySet::Point *p = &vertices[i];
			B.add_vertex(Point(p->x, p->y, p->z));
#ifdef GEN_SURFACE_DEBUG
			printf("%d: %f %f %f\n", i, p->x, p->y, p->z);
#endif
		}

		for (int i = 0; i < ps->polygons.size(); i++) {
			const PolySet::Polygon *poly = &ps->polygons[i];
			QHash<int,int> fc;
			bool facet_is_degenerated = false;
			for (int j = 0; j < poly->size(); j++) {
				const PolySet::Point *p = &poly->at(j);
				int v = vertices_idx.data(p->x, p->y, p->z);
				if (fc[v]++ > 0)
					facet_is_degenerated = true;
			}
			
			if (!facet_is_degenerated)
				B.begin_facet();
#ifdef GEN_SURFACE_DEBUG
			printf("F:");
#endif
			for (int j = 0; j < poly->size(); j++) {
				const PolySet::Point *p = &poly->at(j);
#ifdef GEN_SURFACE_DEBUG
				printf(" %d (%f,%f,%f)", vertices_idx.data(p->x, p->y, p->z), p->x, p->y, p->z);
#endif
				if (!facet_is_degenerated)
					B.add_vertex_to_facet(vertices_idx.data(p->x, p->y, p->z));
			}
#ifdef GEN_SURFACE_DEBUG
			if (facet_is_degenerated)
				printf(" (degenerated)");
			printf("\n");
#endif
			if (!facet_is_degenerated)
				B.end_facet();
		}

#ifdef GEN_SURFACE_DEBUG
		printf("====================\n");
#endif
		B.end_surface();

		#undef PointKey
	}
};

CGAL_Nef_polyhedron PolySet::render_cgal_nef_polyhedron() const
{
	if (this->is2d)
	{
#if 0
		// This version of the code causes problems in some cases.
		// Example testcase: import_dxf("testdata/polygon8.dxf");
		//
		typedef std::list<CGAL_Nef_polyhedron2::Point> point_list_t;
		typedef point_list_t::iterator point_list_it;
		std::list< point_list_t > pdata_point_lists;
		std::list < std::pair < point_list_it, point_list_it > > pdata;
		Grid2d<CGAL_Nef_polyhedron2::Point> grid(GRID_COARSE);

		for (int i = 0; i < this->polygons.size(); i++) {
			pdata_point_lists.push_back(point_list_t());
			for (int j = 0; j < this->polygons[i].size(); j++) {
				double x = this->polygons[i][j].x;
				double y = this->polygons[i][j].y;
				CGAL_Nef_polyhedron2::Point p;
				if (grid.has(x, y)) {
					p = grid.data(x, y);
				} else {
					p = CGAL_Nef_polyhedron2::Point(x, y);
					grid.data(x, y) = p;
				}
				pdata_point_lists.back().push_back(p);
			}
			pdata.push_back(std::make_pair(pdata_point_lists.back().begin(),
					pdata_point_lists.back().end()));
		}

		CGAL_Nef_polyhedron2 N(pdata.begin(), pdata.end(), CGAL_Nef_polyhedron2::POLYGONS);
		return CGAL_Nef_polyhedron(N);
#endif
#if 0
		// This version of the code works fine but is pretty slow.
		//
		CGAL_Nef_polyhedron2 N;
		Grid2d<CGAL_Nef_polyhedron2::Point> grid(GRID_COARSE);

		for (int i = 0; i < this->polygons.size(); i++) {
			std::list<CGAL_Nef_polyhedron2::Point> plist;
			for (int j = 0; j < this->polygons[i].size(); j++) {
				double x = this->polygons[i][j].x;
				double y = this->polygons[i][j].y;
				CGAL_Nef_polyhedron2::Point p;
				if (grid.has(x, y)) {
					p = grid.data(x, y);
				} else {
					p = CGAL_Nef_polyhedron2::Point(x, y);
					grid.data(x, y) = p;
				}
				plist.push_back(p);
			}
			N += CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
		}

		return CGAL_Nef_polyhedron(N);
#endif
#if 1
		// This version of the code does essentially the same thing as the 2nd
		// version but merges some triangles before sending them to CGAL. This adds
		// complexity but speeds up things..
		//
		struct PolyReducer
		{
			Grid2d<int> grid;
			QHash< QPair<int,int>, QPair<int,int> > egde_to_poly;
			QHash< int, CGAL_Nef_polyhedron2::Point > points;
			QHash< int, QList<int> > polygons;
			int poly_n;

			void add_edges(int pn)
			{
				for (int j = 1; j <= this->polygons[pn].size(); j++) {
					int a = this->polygons[pn][j-1];
					int b = this->polygons[pn][j % this->polygons[pn].size()];
					if (a > b) { a = a^b; b = a^b; a = a^b; }
					if (this->egde_to_poly[QPair<int,int>(a, b)].first == 0)
						this->egde_to_poly[QPair<int,int>(a, b)].first = pn;
					else if (this->egde_to_poly[QPair<int,int>(a, b)].second == 0)
						this->egde_to_poly[QPair<int,int>(a, b)].second = pn;
					else
						abort();
				}
			}

			void del_poly(int pn)
			{
				for (int j = 1; j <= this->polygons[pn].size(); j++) {
					int a = this->polygons[pn][j-1];
					int b = this->polygons[pn][j % this->polygons[pn].size()];
					if (a > b) { a = a^b; b = a^b; a = a^b; }
					if (this->egde_to_poly[QPair<int,int>(a, b)].first == pn)
						this->egde_to_poly[QPair<int,int>(a, b)].first = 0;
					if (this->egde_to_poly[QPair<int,int>(a, b)].second == pn)
						this->egde_to_poly[QPair<int,int>(a, b)].second = 0;
				}
				this->polygons.remove(pn);
			}

			PolyReducer(const PolySet *ps) : grid(GRID_COARSE), poly_n(1)
			{
				int point_n = 1;
				for (int i = 0; i < ps->polygons.size(); i++) {
					for (int j = 0; j < ps->polygons[i].size(); j++) {
						double x = ps->polygons[i][j].x;
						double y = ps->polygons[i][j].y;
						if (this->grid.has(x, y)) {
							int idx = this->grid.data(x, y);
							// Filter away two vertices with the same index (due to grid)
							// This could be done in a more general way, but we'd rather redo the entire
							// grid concept instead.
							if (this->polygons[this->poly_n].indexOf(idx) == -1) {
								this->polygons[this->poly_n].append(this->grid.data(x, y));
							}
						} else {
							this->grid.align(x, y) = point_n;
							this->polygons[this->poly_n].append(point_n);
							this->points[point_n] = CGAL_Nef_polyhedron2::Point(x, y);
							point_n++;
						}
					}
					if (this->polygons[this->poly_n].size() >= 3) {
						add_edges(this->poly_n);
						this->poly_n++;
					}
					else {
						this->polygons.remove(this->poly_n);
					}
				}
			}

			int merge(int p1, int p1e, int p2, int p2e)
			{
				for (int i = 1; i < this->polygons[p1].size(); i++) {
					int j = (p1e + i) % this->polygons[p1].size();
					this->polygons[this->poly_n].append(this->polygons[p1][j]);
				}
				for (int i = 1; i < this->polygons[p2].size(); i++) {
					int j = (p2e + i) % this->polygons[p2].size();
					this->polygons[this->poly_n].append(this->polygons[p2][j]);
				}
				del_poly(p1);
				del_poly(p2);
				add_edges(this->poly_n);
				return this->poly_n++;
			}

			void reduce()
			{
				QList<int> work_queue;
				QHashIterator< int, QList<int> > it(polygons);
				while (it.hasNext()) {
					it.next();
					work_queue.append(it.key());
				}
				while (!work_queue.isEmpty()) {
					int poly1_n = work_queue.first();
					work_queue.removeFirst();
					if (!this->polygons.contains(poly1_n))
						continue;
					for (int j = 1; j <= this->polygons[poly1_n].size(); j++) {
						int a = this->polygons[poly1_n][j-1];
						int b = this->polygons[poly1_n][j % this->polygons[poly1_n].size()];
						if (a > b) { a = a^b; b = a^b; a = a^b; }
						if (this->egde_to_poly[QPair<int,int>(a, b)].first != 0 &&
								this->egde_to_poly[QPair<int,int>(a, b)].second != 0) {
							int poly2_n = this->egde_to_poly[QPair<int,int>(a, b)].first +
									this->egde_to_poly[QPair<int,int>(a, b)].second - poly1_n;
							int poly2_edge = -1;
							for (int k = 1; k <= this->polygons[poly2_n].size(); k++) {
								int c = this->polygons[poly2_n][k-1];
								int d = this->polygons[poly2_n][k % this->polygons[poly2_n].size()];
								if (c > d) { c = c^d; d = c^d; c = c^d; }
								if (a == c && b == d) {
									poly2_edge = k-1;
									continue;
								}
								int poly3_n = this->egde_to_poly[QPair<int,int>(c, d)].first +
										this->egde_to_poly[QPair<int,int>(c, d)].second - poly2_n;
								if (poly3_n < 0)
									continue;
								if (poly3_n == poly1_n)
									goto next_poly1_edge;
							}
							work_queue.append(merge(poly1_n, j-1, poly2_n, poly2_edge));
							goto next_poly1;
						}
					next_poly1_edge:;
					}
				next_poly1:;
				}
			}

			CGAL_Nef_polyhedron2 toNef()
			{
				CGAL_Nef_polyhedron2 N;

				QHashIterator< int, QList<int> > it(polygons);
				while (it.hasNext()) {
					it.next();
					std::list<CGAL_Nef_polyhedron2::Point> plist;
					for (int j = 0; j < it.value().size(); j++) {
						int p = it.value()[j];
						plist.push_back(points[p]);
					}
					N += CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
				}

				return N;
			}
		};

		PolyReducer pr(this);
		// printf("Number of polygons before reduction: %d\n", pr.polygons.size());
		pr.reduce();
		// printf("Number of polygons after reduction: %d\n", pr.polygons.size());
		return CGAL_Nef_polyhedron(pr.toNef());
#endif
#if 0
		// This is another experimental version. I should run faster than the above,
		// is a lot simpler and has only one known weakness: Degenerate polygons, which
		// get repaired by GLUTess, might trigger a CGAL crash here. The only
		// known case for this is triangle-with-duplicate-vertex.dxf
		// FIXME: If we just did a projection, we need to recreate the border!
		if (this->polygons.size() > 0) assert(this->borders.size() > 0);
		CGAL_Nef_polyhedron2 N;
		Grid2d<CGAL_Nef_polyhedron2::Point> grid(GRID_COARSE);

		for (int i = 0; i < this->borders.size(); i++) {
			std::list<CGAL_Nef_polyhedron2::Point> plist;
			for (int j = 0; j < this->borders[i].size(); j++) {
				double x = this->borders[i][j].x;
				double y = this->borders[i][j].y;
				CGAL_Nef_polyhedron2::Point p;
				if (grid.has(x, y)) {
					p = grid.data(x, y);
				} else {
					p = CGAL_Nef_polyhedron2::Point(x, y);
					grid.data(x, y) = p;
				}
				plist.push_back(p);		
			}
			// FIXME: If a border (path) has a duplicate vertex in dxf,
			// the CGAL_Nef_polyhedron2 constructor will crash.
			N ^= CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
		}

		return CGAL_Nef_polyhedron(N);

#endif
	}
	else // not (this->is2d)
	{
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
		CGAL_Polyhedron P;
		CGAL_Build_PolySet builder(this);
		P.delegate(builder);
#if 0
		std::cout << P;
#endif
		CGAL_Nef_polyhedron3 N(P);
		return CGAL_Nef_polyhedron(N);
		}
		catch (CGAL::Assertion_exception e) {
			PRINTF("CGAL error: %s", e.what());
			CGAL::set_error_behaviour(old_behaviour);
			return CGAL_Nef_polyhedron();
		}
		CGAL::set_error_behaviour(old_behaviour);
	}
	return CGAL_Nef_polyhedron();
}

#endif /* ENABLE_CGAL */


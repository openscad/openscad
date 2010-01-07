/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#define INCLUDE_ABSTRACT_NODE_DETAILS

#include "openscad.h"
#include "printutils.h"

QCache<QString,PolySet::ps_cache_entry> PolySet::ps_cache(100);

PolySet::ps_cache_entry::ps_cache_entry(PolySet *ps) :
		ps(ps), msg(print_messages_stack.last()) { }

PolySet::ps_cache_entry::~ps_cache_entry() {
	ps->unlink();
}

PolySet::PolySet()
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

static void gl_draw_triangle(GLint *shaderinfo, const PolySet::Point *p0, const PolySet::Point *p1, const PolySet::Point *p2, bool e0, bool e1, bool e2, double z)
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
		glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
		glVertexAttrib3d(shaderinfo[4], p0->x, p0->y, p0->z + z);
		glVertexAttrib3d(shaderinfo[5], p2->x, p2->y, p2->z + z);
		glVertexAttrib3d(shaderinfo[6], 0.0, 0.0, 1.0);
		glVertex3d(p1->x, p1->y, p1->z + z);
		glVertexAttrib3d(shaderinfo[3], e0f, e1f, e2f);
		glVertexAttrib3d(shaderinfo[4], p0->x, p0->y, p0->z + z);
		glVertexAttrib3d(shaderinfo[5], p1->x, p1->y, p1->z + z);
		glVertexAttrib3d(shaderinfo[6], 1.0, 0.0, 0.0);
		glVertex3d(p2->x, p2->y, p2->z + z);
	}
	else
#endif
	{
		glVertex3d(p0->x, p0->y, p0->z + z);
		glVertex3d(p1->x, p1->y, p1->z + z);
		glVertex3d(p2->x, p2->y, p2->z + z);
	}
}

void PolySet::render_surface(colormode_e colormode, csgmode_e csgmode, GLint *shaderinfo) const
{
	if (colormode == COLORMODE_MATERIAL) {
		glColor3ub(249, 215, 44);
#ifdef ENABLE_OPENCSG
		if (shaderinfo) {
			glUniform4f(shaderinfo[1], 249 / 255.0, 215 / 255.0, 44 / 255.0, 1.0);
			glUniform4f(shaderinfo[2], 255 / 255.0, 236 / 255.0, 94 / 255.0, 1.0);
		}
#endif /* ENABLE_OPENCSG */
	}
	if (colormode == COLORMODE_CUTOUT) {
		glColor3ub(157, 203, 81);
#ifdef ENABLE_OPENCSG
		if (shaderinfo) {
			glUniform4f(shaderinfo[1], 157 / 255.0, 203 / 255.0, 81 / 255.0, 1.0);
			glUniform4f(shaderinfo[2], 171 / 255.0, 216 / 255.0, 86 / 255.0, 1.0);
		}
#endif /* ENABLE_OPENCSG */
	}
	if (colormode == COLORMODE_HIGHLIGHT) {
		glColor4ub(255, 157, 81, 128);
#ifdef ENABLE_OPENCSG
		if (shaderinfo) {
			glUniform4f(shaderinfo[1], 255 / 255.0, 157 / 255.0, 81 / 255.0, 0.5);
			glUniform4f(shaderinfo[2], 255 / 255.0, 171 / 255.0, 86 / 255.0, 0.5);
		}
#endif /* ENABLE_OPENCSG */
	}
	if (colormode == COLORMODE_BACKGROUND) {
		glColor4ub(180, 180, 180, 128);
#ifdef ENABLE_OPENCSG
		if (shaderinfo) {
			glUniform4f(shaderinfo[1], 180 / 255.0, 180 / 255.0, 180 / 255.0, 0.5);
			glUniform4f(shaderinfo[2], 150 / 255.0, 150 / 255.0, 150 / 255.0, 0.5);
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
						gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(2), &poly->at(1), true, true, true, z);
					} else {
						gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(1), &poly->at(2), true, true, true, z);
					}
				}
				else if (poly->size() == 4) {
					if (z < 0) {
						gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(3), &poly->at(1), true, false, true, z);
						gl_draw_triangle(shaderinfo, &poly->at(2), &poly->at(1), &poly->at(3), true, false, true, z);
					} else {
						gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(1), &poly->at(3), true, false, true, z);
						gl_draw_triangle(shaderinfo, &poly->at(2), &poly->at(3), &poly->at(1), true, false, true, z);
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
									false, true, false, z);
						} else {
							gl_draw_triangle(shaderinfo, &center, &poly->at(j - 1), &poly->at(j % poly->size()),
									false, true, false, z);
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
				gl_draw_triangle(shaderinfo, &p2, &p1, &p3, true, true, false, 0);
				gl_draw_triangle(shaderinfo, &p2, &p3, &p4, false, true, true, 0);
			}
		}
		glEnd();
	} else {
		for (int i = 0; i < polygons.size(); i++) {
			const Polygon *poly = &polygons[i];
			glBegin(GL_TRIANGLES);
			if (poly->size() == 3) {
				gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(1), &poly->at(2), true, true, true, 0);
			}
			else if (poly->size() == 4) {
				gl_draw_triangle(shaderinfo, &poly->at(0), &poly->at(1), &poly->at(3), true, false, true, 0);
				gl_draw_triangle(shaderinfo, &poly->at(2), &poly->at(3), &poly->at(1), true, false, true, 0);
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
					gl_draw_triangle(shaderinfo, &center, &poly->at(j - 1), &poly->at(j % poly->size()), false, true, false, 0);
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

		QVector<PolySet::Point> vertices;
		Grid3d<int> vertices_idx;

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
				printf(" %d", vertices_idx.data(p->x, p->y, p->z));
#endif
				if (!facet_is_degenerated)
					B.add_vertex_to_facet(vertices_idx.data(p->x, p->y, p->z));
			}
#ifdef GEN_SURFACE_DEBUG
			if (facet_is_degenerated)
				printf(" (degenerated)\n");
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
		typedef std::list<CGAL_Nef_polyhedron2::Point> point_list_t;
		typedef point_list_t::iterator point_list_it;
		std::list< point_list_t > pdata_point_lists;
		std::list < std::pair < point_list_it, point_list_it > > pdata;

		for (int i = 0; i < this->polygons.size(); i++) {
			pdata_point_lists.push_back(point_list_t());
			for (int j = 0; j < this->polygons[i].size(); j++) {
				double x = this->polygons[i][j].x;
				double y = this->polygons[i][j].y;
				CGAL_Nef_polyhedron2::Point p = CGAL_Nef_polyhedron2::Point(x, y);
				pdata_point_lists.back().push_back(p);
			}
			pdata.push_back(std::make_pair(pdata_point_lists.back().begin(),
					pdata_point_lists.back().end()));
		}

		CGAL_Nef_polyhedron2 N(pdata.begin(), pdata.end(), CGAL_Nef_polyhedron2::POLYGONS);
		return CGAL_Nef_polyhedron(N);
	}
	else
	{
		CGAL_Polyhedron P;
		CGAL_Build_PolySet builder(this);
		P.delegate(builder);
#if 0
		std::cout << P;
#endif
		CGAL_Nef_polyhedron3 N(P);
		return CGAL_Nef_polyhedron(N);
	}
	return CGAL_Nef_polyhedron();
}

#endif /* ENABLE_CGAL */

PolySet *AbstractPolyNode::render_polyset(render_mode_e) const
{
	return NULL;
}

#ifdef ENABLE_CGAL

CGAL_Nef_polyhedron AbstractPolyNode::render_cgal_nef_polyhedron() const
{
	QString cache_id = mk_cache_id();
	if (cgal_nef_cache.contains(cache_id)) {
		progress_report();
		PRINT(cgal_nef_cache[cache_id]->msg);
		return cgal_nef_cache[cache_id]->N;
	}

	print_messages_push();

	PolySet *ps = render_polyset(RENDER_CGAL);
	CGAL_Nef_polyhedron N = ps->render_cgal_nef_polyhedron();

	cgal_nef_cache.insert(cache_id, new cgal_nef_cache_entry(N), N.weight());
	print_messages_pop();
	progress_report();

	ps->unlink();
	return N;
}

#endif /* ENABLE_CGAL */

CSGTerm *AbstractPolyNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	PolySet *ps = render_polyset(RENDER_OPENCSG);
	return render_csg_term_from_ps(m, highlights, background, ps, modinst, idx);
}

CSGTerm *AbstractPolyNode::render_csg_term_from_ps(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background, PolySet *ps, const ModuleInstantiation *modinst, int idx)
{
	CSGTerm *t = new CSGTerm(ps, m, QString("n%1").arg(idx));
	if (modinst->tag_highlight && highlights)
		highlights->append(t->link());
	if (modinst->tag_background && background) {
		background->append(t);
		return NULL;
	}
	return t;
}


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

PolySet::PolySet()
{
}

void PolySet::append_poly()
{
	polygons.append(Polygon());
}

void PolySet::append_vertex(double x, double y, double z)
{
	polygons.last().append(Point(x, y, z));
}

void PolySet::insert_vertex(double x, double y, double z)
{
	polygons.last().insert(0, Point(x, y, z));
}

void PolySet::render_surface(colormode_e colormode) const
{
	if (colormode == COLOR_MATERIAL)
		glColor3ub(249, 215, 44);
	if (colormode == COLOR_CUTOUT)
		glColor3ub(157, 203, 81);
	for (int i = 0; i < polygons.size(); i++) {
		const Polygon *poly = &polygons[i];
		glBegin(GL_POLYGON);
		for (int j = 0; j < poly->size(); j++) {
			const Point *p = &poly->at(j);
			glVertex3d(p->x, p->y, p->z);
		}
		glEnd();
	}
}

void PolySet::render_edges(colormode_e colormode) const
{
	if (colormode == COLOR_MATERIAL)
		glColor3ub(255, 236, 94);
	if (colormode == COLOR_CUTOUT)
		glColor3ub(171, 216, 86);
	for (int i = 0; i < polygons.size(); i++) {
		const Polygon *poly = &polygons[i];
		glBegin(GL_LINE_STRIP);
		for (int j = 0; j < poly->size(); j++) {
			const Point *p = &poly->at(j);
			glVertex3d(p->x, p->y, p->z);
		}
		glEnd();
	}
}

#ifdef ENABLE_CGAL

class CGAL_Build_PolySet : public CGAL::Modifier_base<CGAL_HDS>
{
public:
	typedef CGAL_HDS::Vertex::Point Point;

	const PolySet *ps;
	CGAL_Build_PolySet(const PolySet *ps) : ps(ps) { }

	void operator()(CGAL_HDS& hds)
	{
		CGAL_Polybuilder B(hds, true);

		typedef QPair<QPair<double,double>,double> PointKey_t;
		#define PointKey(_x,_y,_z) PointKey_t(QPair<double,double>(_x,_y),_z)

		QVector<PolySet::Point> vertices;
		QHash<PointKey_t,int> vertices_idx;

		for (int i = 0; i < ps->polygons.size(); i++) {
			const PolySet::Polygon *poly = &ps->polygons[i];
			for (int j = 0; j < poly->size(); j++) {
				const PolySet::Point *p = &poly->at(j);
				PointKey_t pk = PointKey(p->x, p->y, p->z);
				if (!vertices_idx.contains(pk)) {
					vertices_idx[pk] = vertices.size();
					vertices.append(*p);
				}
			}
		}

		B.begin_surface(vertices.size(), ps->polygons.size());

		for (int i = 0; i < vertices.size(); i++) {
			const PolySet::Point *p = &vertices[i];
			B.add_vertex(Point(p->x, p->y, p->z));
		}

		for (int i = 0; i < ps->polygons.size(); i++) {
			const PolySet::Polygon *poly = &ps->polygons[i];
			B.begin_facet();
			for (int j = 0; j < poly->size(); j++) {
				const PolySet::Point *p = &poly->at(j);
				PointKey_t pk = PointKey(p->x, p->y, p->z);
				B.add_vertex_to_facet(vertices_idx[pk]);
			}
			B.end_facet();
		}

		B.end_surface();

		#undef PointKey
	}
};

CGAL_Nef_polyhedron PolySet::render_cgal_nef_polyhedron() const
{
	CGAL_Polyhedron P;
	CGAL_Build_PolySet builder(this);
	P.delegate(builder);
#if 0
	std::cout << P;
#endif
	CGAL_Nef_polyhedron N(P);
	return N;
}

#endif /* ENABLE_CGAL */

PolySet *AbstractPolyNode::render_polyset(render_mode_e) const
{
	return NULL;
}

#ifdef ENABLE_CGAL

CGAL_Nef_polyhedron AbstractPolyNode::render_cgal_nef_polyhedron() const
{
	PolySet *ps = render_polyset(RENDER_CGAL);
	CGAL_Nef_polyhedron N = ps->render_cgal_nef_polyhedron();
	progress_report();
	delete ps;
	return N;
}

#endif /* ENABLE_CGAL */

#ifdef ENABLE_OPENCSG

CSGTerm *AbstractPolyNode::render_csg_term(double m[16]) const
{
	PolySet *ps = render_polyset(RENDER_OPENCSG);
	return new CSGTerm(ps, QString("n%1").arg(idx), m);
}

#endif /* ENABLE_OPENCSG */


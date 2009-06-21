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

class CubeModule : public AbstractModule
{
public:
	virtual AbstractNode *evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const;
};

class CubeNode : public AbstractNode
{
public:
	double x, y, z;
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
	virtual QString dump(QString indent) const;
};

AbstractNode *CubeModule::evaluate(const Context*, const QVector<QString>&, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const
{
	CubeNode *node = new CubeNode();
	if (call_argvalues.size() == 1 && call_argvalues[0].is_vector) {
		node->x = call_argvalues[0].x;
		node->y = call_argvalues[0].y;
		node->z = call_argvalues[0].z;
	} else if (call_argvalues.size() == 1 && !call_argvalues[0].is_nan) {
		node->x = call_argvalues[0].x;
		node->y = call_argvalues[0].x;
		node->z = call_argvalues[0].x;
	}
	foreach (AbstractNode *v, child_nodes)
		delete v;
	return node;
}

void register_builtin_cube()
{
	builtin_modules["cube"] = new CubeModule();
}

class CGAL_Build_cube : public CGAL::Modifier_base<CGAL_HDS>
{
public:
	const CubeNode *n;
	CGAL_Build_cube(const CubeNode *n) : n(n) { }
	void operator()(CGAL_HDS& hds) {
		CGAL_Polybuilder B(hds, true);
		B.begin_surface(8, 6, 24);
		typedef CGAL_HDS::Vertex::Point Point;
		B.add_vertex(Point(-n->x/2, -n->y/2, +n->z/2)); // 0
		B.add_vertex(Point(+n->x/2, -n->y/2, +n->z/2)); // 1
		B.add_vertex(Point(+n->x/2, +n->y/2, +n->z/2)); // 2
		B.add_vertex(Point(-n->x/2, +n->y/2, +n->z/2)); // 3
		B.add_vertex(Point(-n->x/2, -n->y/2, -n->z/2)); // 4
		B.add_vertex(Point(+n->x/2, -n->y/2, -n->z/2)); // 5
		B.add_vertex(Point(+n->x/2, +n->y/2, -n->z/2)); // 6
		B.add_vertex(Point(-n->x/2, +n->y/2, -n->z/2)); // 7
		B.begin_facet();
		B.add_vertex_to_facet(0);
		B.add_vertex_to_facet(1);
		B.add_vertex_to_facet(2);
		B.add_vertex_to_facet(3);
		B.end_facet();
		B.begin_facet();
		B.add_vertex_to_facet(7);
		B.add_vertex_to_facet(6);
		B.add_vertex_to_facet(5);
		B.add_vertex_to_facet(4);
		B.end_facet();
		B.begin_facet();
		B.add_vertex_to_facet(4);
		B.add_vertex_to_facet(5);
		B.add_vertex_to_facet(1);
		B.add_vertex_to_facet(0);
		B.end_facet();
		B.begin_facet();
		B.add_vertex_to_facet(5);
		B.add_vertex_to_facet(6);
		B.add_vertex_to_facet(2);
		B.add_vertex_to_facet(1);
		B.end_facet();
		B.begin_facet();
		B.add_vertex_to_facet(6);
		B.add_vertex_to_facet(7);
		B.add_vertex_to_facet(3);
		B.add_vertex_to_facet(2);
		B.end_facet();
		B.begin_facet();
		B.add_vertex_to_facet(7);
		B.add_vertex_to_facet(4);
		B.add_vertex_to_facet(0);
		B.add_vertex_to_facet(3);
		B.end_facet();
		B.end_surface();
	}
};

CGAL_Nef_polyhedron CubeNode::render_cgal_nef_polyhedron() const
{
	CGAL_Polyhedron P;
	CGAL_Build_cube builder(this);
	P.delegate(builder);
	CGAL_Nef_polyhedron N(P);
	progress_report();
	return N;
}

QString CubeNode::dump(QString indent) const
{
	QString text;
	if (x == y && y == z)
		text.sprintf("cube(%f);\n", x);
	else
		text.sprintf("cube([%f %f %f]);\n", x, y, z);
	return indent + text;
}


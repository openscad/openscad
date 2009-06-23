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

enum primitive_type_e {
	CUBE,
	SPHERE,
	CYLINDER
};

class PrimitiveModule : public AbstractModule
{
public:
	primitive_type_e type;
	PrimitiveModule(primitive_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const;
};

class PrimitiveNode : public AbstractNode
{
public:
	bool center;
	double x, y, z, h, r1, r2;
	primitive_type_e type;
	PrimitiveNode(primitive_type_e type) : type(type) { }
	virtual CGAL_Nef_polyhedron render_cgal_nef_polyhedron() const;
	virtual QString dump(QString indent) const;
};

AbstractNode *PrimitiveModule::evaluate(const Context *ctx, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<AbstractNode*> child_nodes) const
{
	PrimitiveNode *node = new PrimitiveNode(type);

	node->center = false;
	node->x = node->y = node->z = node->h = node->r1 = node->r2 = 1;

	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	if (type == CUBE) {
		argnames = QVector<QString>() << "size" << "center";
	}
	if (type == SPHERE) {
		argnames = QVector<QString>() << "r";
	}
	if (type == CYLINDER) {
		argnames = QVector<QString>() << "h" << "r1" << "r2" << "center";
	}

	Context c(ctx);
	c.args(argnames, argexpr, call_argnames, call_argvalues);

	if (type == CUBE) {
		Value size = c.lookup_variable("size");
		Value center = c.lookup_variable("center");
		if (size.type == Value::VECTOR) {
			node->x = size.x;
			node->y = size.y;
			node->z = size.z;
		}
		if (size.type == Value::NUMBER) {
			node->x = size.num;
			node->y = size.num;
			node->z = size.num;
		}
		if (center.type == Value::BOOL) {
			node->center = center.b;
		}
	}

	if (type == SPHERE) {
		Value r = c.lookup_variable("r");
		if (r.type == Value::NUMBER) {
			node->r1 = r.num;
		}
	}

	if (type == CYLINDER) {
		Value h = c.lookup_variable("h");
		Value r = c.lookup_variable("r");
		Value r1 = c.lookup_variable("r1");
		Value r2 = c.lookup_variable("r2");
		Value center = c.lookup_variable("center");
		if (h.type == Value::NUMBER) {
			node->h = h.num;
		}
		if (r.type == Value::NUMBER) {
			node->r1 = r.num;
			node->r2 = r.num;
		}
		if (r1.type == Value::NUMBER) {
			node->r1 = r1.num;
		}
		if (r2.type == Value::NUMBER) {
			node->r2 = r2.num;
		}
		if (center.type == Value::BOOL) {
			node->center = center.b;
		}
	}

	foreach (AbstractNode *v, child_nodes)
		delete v;

	return node;
}

void register_builtin_primitive()
{
	builtin_modules["cube"] = new PrimitiveModule(CUBE);
	builtin_modules["sphere"] = new PrimitiveModule(SPHERE);
	builtin_modules["cylinder"] = new PrimitiveModule(CYLINDER);
}

static int cube_facets[6][4] = {
	{ 0, 1, 2, 3 },
	{ 7, 6, 5, 4 },
	{ 4, 5, 1, 0 },
	{ 5, 6, 2, 1 },
	{ 6, 7, 3, 2 },
	{ 7, 4, 0, 3 }
};

class CGAL_Build_cube : public CGAL::Modifier_base<CGAL_HDS>
{
public:
	typedef CGAL_HDS::Vertex::Point Point;

	const PrimitiveNode *n;
	CGAL_Build_cube(const PrimitiveNode *n) : n(n) { }

	void operator()(CGAL_HDS& hds)
	{
		CGAL_Polybuilder B(hds, true);

		if (n->type == CUBE) {
			B.begin_surface(8, 6, 24);
			double x1, x2, y1, y2, z1, z2;
			if (n->center) {
				x1 = -n->x/2;
				x2 = +n->x/2;
				y1 = -n->y/2;
				y2 = +n->y/2;
				z1 = -n->z/2;
				z2 = +n->z/2;
			} else {
				x1 = y1 = z1 = 0;
				x2 = n->x;
				y2 = n->y;
				z2 = n->z;
			}
			B.add_vertex(Point(x1, y1, z2)); // 0
			B.add_vertex(Point(x2, y1, z2)); // 1
			B.add_vertex(Point(x2, y2, z2)); // 2
			B.add_vertex(Point(x1, y2, z2)); // 3
			B.add_vertex(Point(x1, y1, z1)); // 4
			B.add_vertex(Point(x2, y1, z1)); // 5
			B.add_vertex(Point(x2, y2, z1)); // 6
			B.add_vertex(Point(x1, y2, z1)); // 7
			for (int i = 0; i < 6; i++) {
				B.begin_facet();
				for (int j = 0; j < 4; j++)
					B.add_vertex_to_facet(cube_facets[i][j]);
				B.end_facet();
			}
			B.end_surface();
		}

		if (n->type == SPHERE) {
			/* FIXME */
		}

		if (n->type == CYLINDER) {
			int fragments = 10;
			B.begin_surface(fragments*2, fragments*2+2);
			double z1, z2;
			if (n->center) {
				z1 = -n->h/2;
				z2 = +n->h/2;
			} else {
				z1 = 0;
				z2 = n->h;
			}
			for (int i=0; i<fragments; i++) {
				double phi = (M_PI*2*i) / fragments;
				B.add_vertex(Point(n->r1*cos(phi), n->r1*sin(phi), z1));
				B.add_vertex(Point(n->r2*cos(phi), n->r2*sin(phi), z2));
			}
			for (int i=0; i<fragments; i++) {
				B.begin_facet();
				B.add_vertex_to_facet(i*2);
				B.add_vertex_to_facet(i*2+1);
				B.add_vertex_to_facet(((i+1)%fragments)*2);
				B.end_facet();
				B.begin_facet();
				B.add_vertex_to_facet(i*2+1);
				B.add_vertex_to_facet(((i+1)%fragments)*2+1);
				B.add_vertex_to_facet(((i+1)%fragments)*2);
				B.end_facet();
			}
			B.begin_facet();
			for (int i=0; i<fragments; i++) {
				B.add_vertex_to_facet(i*2);
			}
			B.end_facet();
			B.begin_facet();
			for (int i=fragments-1; i>=0; i--) {
				B.add_vertex_to_facet(i*2+1);
			}
			B.end_facet();
			B.end_surface();
		}
	}
};

CGAL_Nef_polyhedron PrimitiveNode::render_cgal_nef_polyhedron() const
{
	CGAL_Polyhedron P;
	CGAL_Build_cube builder(this);
	P.delegate(builder);
#if 0
	std::cout << P;
#endif
	CGAL_Nef_polyhedron N(P);
	progress_report();
	return N;
}

QString PrimitiveNode::dump(QString indent) const
{
	QString text;
	if (type == CUBE)
		text.sprintf("cube(size = [%f %f %f], center = %s);\n", x, y, z, center ? "true" : "false");
	if (type == SPHERE)
		text.sprintf("sphere(r = %f);\n", r1);
	if (type == CYLINDER)
		text.sprintf("cylinder(h = %f, r1 = %f, r2 = %f, center = %s);\n", h, r1, r2, center ? "true" : "false");
	return indent + text;
}


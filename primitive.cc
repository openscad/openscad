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

class PrimitiveNode : public AbstractPolyNode
{
public:
	bool center;
	double x, y, z, h, r1, r2;
	primitive_type_e type;
	PrimitiveNode(primitive_type_e type) : type(type) { }
	virtual PolySet *render_polyset(render_mode_e mode) const;
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


PolySet *PrimitiveNode::render_polyset(render_mode_e mode) const
{
	PolySet *p = new PolySet();

	if (type == CUBE && x > 0 && y > 0 && z > 0)
	{
		double x1, x2, y1, y2, z1, z2;
		if (center) {
			x1 = -x/2;
			x2 = +x/2;
			y1 = -y/2;
			y2 = +y/2;
			z1 = -z/2;
			z2 = +z/2;
		} else {
			x1 = y1 = z1 = 0;
			x2 = x;
			y2 = y;
			z2 = z;
		}

		p->append_poly(); // top
		p->append_vertex(x1, y1, z2);
		p->append_vertex(x2, y1, z2);
		p->append_vertex(x2, y2, z2);
		p->append_vertex(x1, y2, z2);

		p->append_poly(); // bottom
		p->append_vertex(x1, y2, z1);
		p->append_vertex(x2, y2, z1);
		p->append_vertex(x2, y1, z1);
		p->append_vertex(x1, y1, z1);

		p->append_poly(); // side1
		p->append_vertex(x1, y1, z1);
		p->append_vertex(x2, y1, z1);
		p->append_vertex(x2, y1, z2);
		p->append_vertex(x1, y1, z2);

		p->append_poly(); // side2
		p->append_vertex(x2, y1, z1);
		p->append_vertex(x2, y2, z1);
		p->append_vertex(x2, y2, z2);
		p->append_vertex(x2, y1, z2);

		p->append_poly(); // side3
		p->append_vertex(x2, y2, z1);
		p->append_vertex(x1, y2, z1);
		p->append_vertex(x1, y2, z2);
		p->append_vertex(x2, y2, z2);

		p->append_poly(); // side4
		p->append_vertex(x1, y2, z1);
		p->append_vertex(x1, y1, z1);
		p->append_vertex(x1, y1, z2);
		p->append_vertex(x1, y2, z2);
	}

	if (type == SPHERE && r1 > 0)
	{
		/* FIXME */
	}

	if (type == CYLINDER && h > 0 && r1 >=0 && r2 >= 0 && (r1 > 0 || r2 > 0))
	{
		int fragments = 10;

		double z1, z2;
		if (center) {
			z1 = -h/2;
			z2 = +h/2;
		} else {
			z1 = 0;
			z2 = h;
		}

		struct point2d {
			double x, y;
		};

		point2d circle1[fragments];
		point2d circle2[fragments];

		for (int i=0; i<fragments; i++) {
			double phi = (M_PI*2*i) / fragments;
			circle1[i].x = r1*cos(phi);
			circle1[i].y = r1*sin(phi);
			circle2[i].x = r2*cos(phi);
			circle2[i].y = r2*sin(phi);
		}
		
		for (int i=0; i<fragments; i++) {
			int j = (i+1) % fragments;
			p->append_poly();
			p->append_vertex(circle1[i].x, circle1[i].y, z1);
			p->append_vertex(circle2[i].x, circle2[i].y, z2);
			p->append_vertex(circle1[j].x, circle1[j].y, z1);
			p->append_poly();
			p->append_vertex(circle2[i].x, circle2[i].y, z2);
			p->append_vertex(circle2[j].x, circle2[j].y, z2);
			p->append_vertex(circle1[j].x, circle1[j].y, z1);
		}

		if (r1 > 0) {
			p->append_poly();
			for (int i=0; i<fragments; i++)
				p->append_vertex(circle1[i].x, circle1[i].y, z1);
		}

		if (r2 > 0) {
			p->append_poly();
			for (int i=0; i<fragments; i++)
				p->insert_vertex(circle2[i].x, circle2[i].y, z2);
		}
	}

	return p;
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
	return indent + QString("n%1: ").arg(idx) + text;
}


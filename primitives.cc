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
	CYLINDER,
	POLYHEDRON,
	SQUARE,
	CIRCLE
};

class PrimitiveModule : public AbstractModule
{
public:
	primitive_type_e type;
	PrimitiveModule(primitive_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

class PrimitiveNode : public AbstractPolyNode
{
public:
	bool center;
	double x, y, z, h, r1, r2;
	double fn, fs, fa;
	primitive_type_e type;
	Value points, triangles;
	PrimitiveNode(const ModuleInstantiation *mi, primitive_type_e type) : AbstractPolyNode(mi), type(type) { }
	virtual PolySet *render_polyset(render_mode_e mode) const;
	virtual QString dump(QString indent) const;
};

AbstractNode *PrimitiveModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	PrimitiveNode *node = new PrimitiveNode(inst, type);

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
	if (type == POLYHEDRON) {
		argnames = QVector<QString>() << "points" << "triangles";
	}
	if (type == SQUARE) {
		argnames = QVector<QString>() << "size" << "center";
	}
	if (type == CIRCLE) {
		argnames = QVector<QString>() << "r";
	}

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	node->fn = c.lookup_variable("$fn").num;
	node->fs = c.lookup_variable("$fs").num;
	node->fa = c.lookup_variable("$fa").num;

	if (type == CUBE) {
		Value size = c.lookup_variable("size");
		Value center = c.lookup_variable("center");
		size.getnum(node->x);
		size.getnum(node->y);
		size.getnum(node->z);
		size.getv3(node->x, node->y, node->z);
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
		Value r, r1, r2;
		r1 = c.lookup_variable("r1");
		r2 = c.lookup_variable("r2");
		if (r1.type != Value::NUMBER && r1.type != Value::NUMBER)
			r = c.lookup_variable("r");
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

	if (type == POLYHEDRON) {
		node->points = c.lookup_variable("points");
		node->triangles = c.lookup_variable("triangles");
	}

	if (type == SQUARE) {
		Value size = c.lookup_variable("size");
		Value center = c.lookup_variable("center");
		size.getnum(node->x);
		size.getnum(node->y);
		size.getv2(node->x, node->y);
		if (center.type == Value::BOOL) {
			node->center = center.b;
		}
	}

	if (type == CIRCLE) {
		Value r = c.lookup_variable("r");
		if (r.type == Value::NUMBER) {
			node->r1 = r.num;
		}
	}

	return node;
}

void register_builtin_primitives()
{
	builtin_modules["cube"] = new PrimitiveModule(CUBE);
	builtin_modules["sphere"] = new PrimitiveModule(SPHERE);
	builtin_modules["cylinder"] = new PrimitiveModule(CYLINDER);
	builtin_modules["polyhedron"] = new PrimitiveModule(POLYHEDRON);
	builtin_modules["square"] = new PrimitiveModule(SQUARE);
	builtin_modules["circle"] = new PrimitiveModule(CIRCLE);
}

int get_fragments_from_r(double r, double fn, double fs, double fa)
{
	if (fn > 0.0)
		return (int)fn;
	return (int)ceil(fmax(fmin(360.0 / fa, r*M_PI / fs), 5));
}

PolySet *PrimitiveNode::render_polyset(render_mode_e) const
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
		struct point2d {
			double x, y;
		};

		struct ring_s {
			int fragments;
			point2d *points;
			double r, z;
		};

		int rings = get_fragments_from_r(r1, fn, fs, fa);
		ring_s ring[rings];

		for (int i = 0; i < rings; i++) {
			double phi = (M_PI * (i + 0.5)) / rings;
			ring[i].r = r1 * sin(phi);
			ring[i].z = r1 * cos(phi);
			ring[i].fragments = get_fragments_from_r(ring[i].r, fn, fs, fa);
			ring[i].points = new point2d[ring[i].fragments];
			for (int j = 0; j < ring[i].fragments; j++) {
				phi = (M_PI*2*j) / ring[i].fragments;
				ring[i].points[j].x = ring[i].r * cos(phi);
				ring[i].points[j].y = ring[i].r * sin(phi);
			}
		}

		p->append_poly();
		for (int i = 0; i < ring[0].fragments; i++)
			p->append_vertex(ring[0].points[i].x, ring[0].points[i].y, ring[0].z);

		for (int i = 0; i < rings-1; i++) {
			ring_s *r1 = &ring[i];
			ring_s *r2 = &ring[i+1];
			int r1i = 0, r2i = 0;
			while (r1i < r1->fragments || r2i < r2->fragments)
			{
				if (r1i >= r1->fragments)
					goto sphere_next_r2;
				if (r2i >= r2->fragments)
					goto sphere_next_r1;
				if ((double)r1i / r1->fragments <
						(double)r2i / r2->fragments)
				{
sphere_next_r1:
					p->append_poly();
					int r1j = (r1i+1) % r1->fragments;
					p->insert_vertex(r1->points[r1i].x, r1->points[r1i].y, r1->z);
					p->insert_vertex(r1->points[r1j].x, r1->points[r1j].y, r1->z);
					p->insert_vertex(r2->points[r2i % r2->fragments].x, r2->points[r2i % r2->fragments].y, r2->z);
					r1i++;
				} else {
sphere_next_r2:
					p->append_poly();
					int r2j = (r2i+1) % r2->fragments;
					p->append_vertex(r2->points[r2i].x, r2->points[r2i].y, r2->z);
					p->append_vertex(r2->points[r2j].x, r2->points[r2j].y, r2->z);
					p->append_vertex(r1->points[r1i % r1->fragments].x, r1->points[r1i % r1->fragments].y, r1->z);
					r2i++;
				}
			}
		}

		p->append_poly();
		for (int i = 0; i < ring[rings-1].fragments; i++)
			p->insert_vertex(ring[rings-1].points[i].x, ring[rings-1].points[i].y, ring[rings-1].z);
	}

	if (type == CYLINDER && h > 0 && r1 >=0 && r2 >= 0 && (r1 > 0 || r2 > 0))
	{
		int fragments = get_fragments_from_r(fmax(r1, r2), fn, fs, fa);

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
			if (r1 > 0) {
				circle1[i].x = r1*cos(phi);
				circle1[i].y = r1*sin(phi);
			} else {
				circle1[i].x = 0;
				circle1[i].y = 0;
			}
			if (r2 > 0) {
				circle2[i].x = r2*cos(phi);
				circle2[i].y = r2*sin(phi);
			} else {
				circle2[i].x = 0;
				circle2[i].y = 0;
			}
		}
		
		for (int i=0; i<fragments; i++) {
			int j = (i+1) % fragments;
			if (r1 > 0) {
				p->append_poly();
				p->insert_vertex(circle1[i].x, circle1[i].y, z1);
				p->insert_vertex(circle2[i].x, circle2[i].y, z2);
				p->insert_vertex(circle1[j].x, circle1[j].y, z1);
			}
			if (r2 > 0) {
				p->append_poly();
				p->insert_vertex(circle2[i].x, circle2[i].y, z2);
				p->insert_vertex(circle2[j].x, circle2[j].y, z2);
				p->insert_vertex(circle1[j].x, circle1[j].y, z1);
			}
		}

		if (r1 > 0) {
			p->append_poly();
			for (int i=0; i<fragments; i++)
				p->insert_vertex(circle1[i].x, circle1[i].y, z1);
		}

		if (r2 > 0) {
			p->append_poly();
			for (int i=0; i<fragments; i++)
				p->append_vertex(circle2[i].x, circle2[i].y, z2);
		}
	}

	if (type == POLYHEDRON)
	{
		for (int i=0; i<triangles.vec.size(); i++)
		{
			p->append_poly();
			for (int j=0; j<triangles.vec[i]->vec.size(); j++) {
				int pt = triangles.vec[i]->vec[j]->num;
				double px = points.vec[pt]->vec[0]->num;
				double py = points.vec[pt]->vec[1]->num;
				double pz = points.vec[pt]->vec[2]->num;
				p->insert_vertex(px, py, pz);
			}
		}
	}

	if (type == SQUARE)
	{
		double x1, x2, y1, y2;
		if (center) {
			x1 = -x/2;
			x2 = +x/2;
			y1 = -y/2;
			y2 = +y/2;
		} else {
			x1 = y1 = 0;
			x2 = x;
			y2 = y;
		}

		p->is2d = true;
		p->append_poly();
		p->append_vertex(x1, y1);
		p->append_vertex(x2, y1);
		p->append_vertex(x2, y2);
		p->append_vertex(x1, y2);
	}

	if (type == CIRCLE)
	{
		int fragments = get_fragments_from_r(r1, fn, fs, fa);

		struct point2d {
			double x, y;
		};

		point2d circle[fragments];

		for (int i=0; i<fragments; i++) {
			double phi = (M_PI*2*i) / fragments;
			circle[i].x = r1*cos(phi);
			circle[i].y = r1*sin(phi);
		}

		p->is2d = true;
		p->append_poly();
		for (int i=0; i<fragments; i++)
			p->append_vertex(circle[i].x, circle[i].y);
	}

	return p;
}

QString PrimitiveNode::dump(QString indent) const
{
	if (dump_cache.isEmpty()) {
		QString text;
		if (type == CUBE)
			text.sprintf("cube(size = [%f, %f, %f], center = %s);\n", x, y, z, center ? "true" : "false");
		if (type == SPHERE)
			text.sprintf("sphere($fn = %f, $fa = %f, $fs = %f, r = %f);\n", fn, fa, fs, r1);
		if (type == CYLINDER)
			text.sprintf("cylinder($fn = %f, $fa = %f, $fs = %f, h = %f, r1 = %f, r2 = %f, center = %s);\n", fn, fa, fs, h, r1, r2, center ? "true" : "false");
		if (type == POLYHEDRON)
			text.sprintf("polyhedron(points = %s, triangles = %s);\n", points.dump().toAscii().data(), triangles.dump().toAscii().data());
		if (type == SQUARE)
			text.sprintf("square(size = [%f, %f], center = %s);\n", x, y, center ? "true" : "false");
		((AbstractNode*)this)->dump_cache = indent + QString("n%1: ").arg(idx) + text;
	}
	return dump_cache;
}


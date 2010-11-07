/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
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

#include "module.h"
#include "node.h"
#include "polyset.h"
#include "context.h"
#include "dxfdata.h"
#include "dxftess.h"
#include "builtin.h"
#include "printutils.h"
#include <assert.h>
#include "visitor.h"
#include <sstream>
#include <assert.h>

enum primitive_type_e {
	CUBE,
	SPHERE,
	CYLINDER,
	POLYHEDRON,
	SQUARE,
	CIRCLE,
	POLYGON
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
	PrimitiveNode(const ModuleInstantiation *mi, primitive_type_e type) : AbstractPolyNode(mi), type(type) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const {
		switch (this->type) {
		case CUBE:
			return "cube";
			break;
		case SPHERE:
			return "sphere";
			break;
		case CYLINDER:
			return "cylinder";
			break;
		case POLYHEDRON:
			return "polyhedron";
			break;
		case SQUARE:
			return "square";
			break;
		case CIRCLE:
			return "circle";
			break;
		case POLYGON:
			return "polygon";
			break;
		default:
			assert(false && "PrimitiveNode::name(): Unknown primitive type");
			return AbstractPolyNode::name();
		}
	}

	bool center;
	double x, y, z, h, r1, r2;
	double fn, fs, fa;
	primitive_type_e type;
	int convexity;
	Value points, paths, triangles;
	virtual PolySet *render_polyset(render_mode_e mode, class PolySetRenderer *) const;
};

AbstractNode *PrimitiveModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	PrimitiveNode *node = new PrimitiveNode(inst, this->type);

	node->center = false;
	node->x = node->y = node->z = node->h = node->r1 = node->r2 = 1;

	QVector<QString> argnames;
	QVector<Expression*> argexpr;

	switch (this->type) {
	case CUBE:
		argnames = QVector<QString>() << "size" << "center";
		break;
	case SPHERE:
		argnames = QVector<QString>() << "r";
		break;
	case CYLINDER:
		argnames = QVector<QString>() << "h" << "r1" << "r2" << "center";
		break;
	case POLYHEDRON:
		argnames = QVector<QString>() << "points" << "triangles" << "convexity";
		break;
	case SQUARE:
		argnames = QVector<QString>() << "size" << "center";
		break;
	case CIRCLE:
		argnames = QVector<QString>() << "r";
		break;
	case POLYGON:
		argnames = QVector<QString>() << "points" << "paths" << "convexity";
		break;
	default:
		assert(false && "PrimitiveModule::evaluate(): Unknown node type");
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
		if (r1.type != Value::NUMBER && r2.type != Value::NUMBER)
			r = c.lookup_variable("r", true); // silence warning since r has no default value
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

	if (type == POLYGON) {
		node->points = c.lookup_variable("points");
		node->paths = c.lookup_variable("paths");
	}

	node->convexity = c.lookup_variable("convexity", true).num;
	if (node->convexity < 1)
		node->convexity = 1;

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
	builtin_modules["polygon"] = new PrimitiveModule(POLYGON);
}

/*!
	Returns the number of subdivision of a whole circle, given radius and
	the three special variables $fn, $fs and $fa
*/
int get_fragments_from_r(double r, double fn, double fs, double fa)
{
	if (r < GRID_FINE) return 0;
	if (fn > 0.0)
		return (int)fn;
	return (int)ceil(fmax(fmin(360.0 / fa, r*M_PI / fs), 5));
}

PolySet *PrimitiveNode::render_polyset(render_mode_e, class PolySetRenderer *) const
{
	PolySet *p = new PolySet();

	if (this->type == CUBE && this->x > 0 && this->y > 0 && this->z > 0)
	{
		double x1, x2, y1, y2, z1, z2;
		if (this->center) {
			x1 = -this->x/2;
			x2 = +this->x/2;
			y1 = -this->y/2;
			y2 = +this->y/2;
			z1 = -this->z/2;
			z2 = +this->z/2;
		} else {
			x1 = y1 = z1 = 0;
			x2 = this->x;
			y2 = this->y;
			z2 = this->z;
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

	if (this->type == SPHERE && this->r1 > 0)
	{
		struct point2d {
			double x, y;
		};

		struct ring_s {
			int fragments;
			point2d *points;
			double r, z;
		};

		int rings = get_fragments_from_r(this->r1, this->fn, this->fs, this->fa);
		ring_s ring[rings];

		for (int i = 0; i < rings; i++) {
			double phi = (M_PI * (i + 0.5)) / rings;
			ring[i].r = this->r1 * sin(phi);
			ring[i].z = this->r1 * cos(phi);
			ring[i].fragments = get_fragments_from_r(ring[i].r, this->fn, this->fs, this->fa);
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
				if ((double)r1i / r1->fragments < (double)r2i / r2->fragments)
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

	if (this->type == CYLINDER && 
			this->h > 0 && this->r1 >=0 && this->r2 >= 0 && (this->r1 > 0 || this->r2 > 0))
	{
		int fragments = get_fragments_from_r(fmax(this->r1, this->r2), this->fn, this->fs, this->fa);

		double z1, z2;
		if (this->center) {
			z1 = -this->h/2;
			z2 = +this->h/2;
		} else {
			z1 = 0;
			z2 = this->h;
		}

		struct point2d {
			double x, y;
		};

		point2d circle1[fragments];
		point2d circle2[fragments];

		for (int i=0; i<fragments; i++) {
			double phi = (M_PI*2*i) / fragments;
			if (this->r1 > 0) {
				circle1[i].x = this->r1*cos(phi);
				circle1[i].y = this->r1*sin(phi);
			} else {
				circle1[i].x = 0;
				circle1[i].y = 0;
			}
			if (this->r2 > 0) {
				circle2[i].x = this->r2*cos(phi);
				circle2[i].y = this->r2*sin(phi);
			} else {
				circle2[i].x = 0;
				circle2[i].y = 0;
			}
		}
		
		for (int i=0; i<fragments; i++) {
			int j = (i+1) % fragments;
			if (this->r1 > 0) {
				p->append_poly();
				p->insert_vertex(circle1[i].x, circle1[i].y, z1);
				p->insert_vertex(circle2[i].x, circle2[i].y, z2);
				p->insert_vertex(circle1[j].x, circle1[j].y, z1);
			}
			if (this->r2 > 0) {
				p->append_poly();
				p->insert_vertex(circle2[i].x, circle2[i].y, z2);
				p->insert_vertex(circle2[j].x, circle2[j].y, z2);
				p->insert_vertex(circle1[j].x, circle1[j].y, z1);
			}
		}

		if (this->r1 > 0) {
			p->append_poly();
			for (int i=0; i<fragments; i++)
				p->insert_vertex(circle1[i].x, circle1[i].y, z1);
		}

		if (this->r2 > 0) {
			p->append_poly();
			for (int i=0; i<fragments; i++)
				p->append_vertex(circle2[i].x, circle2[i].y, z2);
		}
	}

	if (this->type == POLYHEDRON)
	{
		p->convexity = this->convexity;
		for (int i=0; i<this->triangles.vec.size(); i++)
		{
			p->append_poly();
			for (int j=0; j<this->triangles.vec[i]->vec.size(); j++) {
				int pt = this->triangles.vec[i]->vec[j]->num;
				if (pt < this->points.vec.size()) {
					double px, py, pz;
					if (this->points.vec[pt]->getv3(px, py, pz))
						p->insert_vertex(px, py, pz);
				}
			}
		}
	}

	if (this->type == SQUARE)
	{
		double x1, x2, y1, y2;
		if (this->center) {
			x1 = -this->x/2;
			x2 = +this->x/2;
			y1 = -this->y/2;
			y2 = +this->y/2;
		} else {
			x1 = y1 = 0;
			x2 = this->x;
			y2 = this->y;
		}

		p->is2d = true;
		p->append_poly();
		p->append_vertex(x1, y1);
		p->append_vertex(x2, y1);
		p->append_vertex(x2, y2);
		p->append_vertex(x1, y2);
	}

	if (this->type == CIRCLE)
	{
		int fragments = get_fragments_from_r(this->r1, this->fn, this->fs, this->fa);

		struct point2d {
			double x, y;
		};

		point2d circle[fragments];

		for (int i=0; i<fragments; i++) {
			double phi = (M_PI*2*i) / fragments;
			circle[i].x = this->r1*cos(phi);
			circle[i].y = this->r1*sin(phi);
		}

		p->is2d = true;
		p->append_poly();
		for (int i=0; i<fragments; i++)
			p->append_vertex(circle[i].x, circle[i].y);
	}

	if (this->type == POLYGON)
	{
		DxfData dd;

		for (int i=0; i<this->points.vec.size(); i++) {
			double x,y;
			if (!this->points.vec[i]->getv2(x, y)) {
				PRINTF("ERROR: Unable to convert point at index %d to a vec2 of numbers", i);
				p->unlink();
				return NULL;
			}
			dd.points.append(DxfData::Point(x, y));
		}

		if (this->paths.vec.size() == 0)
		{
			dd.paths.append(DxfData::Path());
			for (int i=0; i<this->points.vec.size(); i++) {
				assert(i < dd.points.size()); // FIXME: Not needed, but this used to be an 'if'
				DxfData::Point *p = &dd.points[i];
				dd.paths.last().points.append(p);
			}
			if (dd.paths.last().points.size() > 0) {
				dd.paths.last().points.append(dd.paths.last().points.first());
				dd.paths.last().is_closed = true;
			}
		}
		else
		{
			for (int i=0; i<this->paths.vec.size(); i++)
			{
				dd.paths.append(DxfData::Path());
				for (int j=0; j<this->paths.vec[i]->vec.size(); j++) {
					int idx = this->paths.vec[i]->vec[j]->num;
					if (idx < dd.points.size()) {
						DxfData::Point *p = &dd.points[idx];
						dd.paths.last().points.append(p);
					}
				}
				if (dd.paths.last().points.isEmpty()) {
					dd.paths.removeLast();
				} else {
					dd.paths.last().points.append(dd.paths.last().points.first());
					dd.paths.last().is_closed = true;
				}
			}
		}

		p->is2d = true;
		p->convexity = convexity;
		dxf_tesselate(p, &dd, 0, true, false, 0);
		dxf_border_to_ps(p, &dd);
	}

	return p;
}

std::string PrimitiveNode::toString() const
{
	std::stringstream stream;

	stream << this->name();

	switch (this->type) {
	case CUBE:
		stream << "(size = [" << this->x << ", " << this->y << ", " << this->z << "], "
					 <<	"center = " << (center ? "true" : "false") << ")";
		break;
	case SPHERE:
		stream << "($fn = " << this->fn << ", $fa = " << this->fa
					 << ", $fs = " << this->fs << ", r = " << this->r1 << ")";
			break;
	case CYLINDER:
		stream << "($fn = " << this->fn << ", $fa = " << this->fa
					 << ", $fs = " << this->fs << ", h = " << this->h << ", r1 = " << this->r1
					 << ", r2 = " << this->r2 << ", center = " << (center ? "true" : "false") << ")";
			break;
	case POLYHEDRON:
		stream << "(points = " << this->points
					 << ", triangles = " << this->triangles
					 << ", convexity = " << this->convexity << ")";
			break;
	case SQUARE:
		stream << "(size = [" << this->x << ", " << this->y << "], "
					 << "center = " << (center ? "true" : "false") << ")";
			break;
	case CIRCLE:
		stream << "($fn = " << this->fn << ", $fa = " << this->fa
					 << ", $fs = " << this->fs << ", r = " << this->r1 << ")";
		break;
	case POLYGON:
		stream << "(points = " << this->points << ", paths = " << this->paths << ", convexity = " << this->convexity << ")";
			break;
	default:
		assert(false);
	}

	return stream.str();
}

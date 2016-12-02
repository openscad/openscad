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

#include "module.h"
#include "evalcontext.h"
#include "builtin.h"
#include "printutils.h"
#include "context.h"
#include "primitivenode.h"
#include <sstream>
#include <assert.h>
#include <cmath>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#define F_MINIMUM 0.01

class PrimitiveModule : public AbstractModule
{
public:
	primitive_type_e type;
	PrimitiveModule(primitive_type_e type) : type(type) { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;
private:
	Value lookup_radius(const Context &ctx, const std::string &radius_var, const std::string &diameter_var) const;
};

/**
 * Return a radius value by looking up both a diameter and radius variable.
 * The diameter has higher priority, so if found an additionally set radius
 * value is ignored.
 * 
 * @param ctx data context with variable values.
 * @param radius_var name of the variable to lookup for the radius value.
 * @param diameter_var name of the variable to lookup for the diameter value.
 * @return radius value of type Value::NUMBER or Value::UNDEFINED if both
 *         variables are invalid or not set.
 */
Value PrimitiveModule::lookup_radius(const Context &ctx, const std::string &diameter_var, const std::string &radius_var) const
{
	ValuePtr d = ctx.lookup_variable(diameter_var, true);
	ValuePtr r = ctx.lookup_variable(radius_var, true);
	const bool r_defined = (r->type() == Value::NUMBER);
	
	if (d->type() == Value::NUMBER) {
		if (r_defined) {
			PRINTB("WARNING: Ignoring radius variable '%s' as diameter '%s' is defined too.", radius_var % diameter_var);
		}
		return Value(d->toDouble() / 2.0);
	} else if (r_defined) {
		return *r;
	} else {
		return Value::undefined;
	}
}

AbstractNode *PrimitiveModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	PrimitiveNode *node = new PrimitiveNode(inst, this->type);

	node->center = false;
	node->x = node->y = node->z = node->h = node->r1 = node->r2 = 1;

	AssignmentList args;

	switch (this->type) {
	case CUBE:
		args += Assignment("size"), Assignment("center");
		break;
	case SPHERE:
		args += Assignment("r");
		break;
	case CYLINDER:
		args += Assignment("h"), Assignment("r1"), Assignment("r2"), Assignment("center");
		break;
	case POLYHEDRON:
		args += Assignment("points"), Assignment("faces"), Assignment("convexity");
		break;
	case SQUARE:
		args += Assignment("size"), Assignment("center");
		break;
	case CIRCLE:
		args += Assignment("r");
		break;
	case POLYGON:
		args += Assignment("points"), Assignment("paths"), Assignment("convexity");
		break;
	default:
		assert(false && "PrimitiveModule::instantiate(): Unknown node type");
	}

	Context c(ctx);
	c.setVariables(args, evalctx);

	node->fn = c.lookup_variable("$fn")->toDouble();
	node->fs = c.lookup_variable("$fs")->toDouble();
	node->fa = c.lookup_variable("$fa")->toDouble();

	if (node->fs < F_MINIMUM) {
		PRINTB("WARNING: $fs too small - clamping to %f", F_MINIMUM);
		node->fs = F_MINIMUM;
	}
	if (node->fa < F_MINIMUM) {
		PRINTB("WARNING: $fa too small - clamping to %f", F_MINIMUM);
		node->fa = F_MINIMUM;
	}

	switch (this->type)  {
	case CUBE: {
		ValuePtr size = c.lookup_variable("size");
		ValuePtr center = c.lookup_variable("center");
		size->getDouble(node->x);
		size->getDouble(node->y);
		size->getDouble(node->z);
		size->getVec3(node->x, node->y, node->z);
		if (center->type() == Value::BOOL) {
			node->center = center->toBool();
		}
		break;
	}
	case SPHERE: {
		const Value r = lookup_radius(c, "d", "r");
		if (r.type() == Value::NUMBER) {
			node->r1 = r.toDouble();
		}
		break;
	}
	case CYLINDER: {
		ValuePtr h = c.lookup_variable("h");
		if (h->type() == Value::NUMBER) {
			node->h = h->toDouble();
		}

		const Value r = lookup_radius(c, "d", "r");
		const Value r1 = lookup_radius(c, "d1", "r1");
		const Value r2 = lookup_radius(c, "d2", "r2");
		if (r.type() == Value::NUMBER) {
			node->r1 = r.toDouble();
			node->r2 = r.toDouble();
		}
		if (r1.type() == Value::NUMBER) {
			node->r1 = r1.toDouble();
		}
		if (r2.type() == Value::NUMBER) {
			node->r2 = r2.toDouble();
		}
		
		ValuePtr center = c.lookup_variable("center");
		if (center->type() == Value::BOOL) {
			node->center = center->toBool();
		}
		break;
	}
	case POLYHEDRON: {
		node->points = c.lookup_variable("points");
		node->faces = c.lookup_variable("faces");
		if (node->faces->type() == Value::UNDEFINED) {
			// backwards compatible
			node->faces = c.lookup_variable("triangles", true);
			if (node->faces->type() != Value::UNDEFINED) {
				printDeprecation("polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
			}
		}
		break;
	}
	case SQUARE: {
		ValuePtr size = c.lookup_variable("size");
		ValuePtr center = c.lookup_variable("center");
		size->getDouble(node->x);
		size->getDouble(node->y);
		size->getVec2(node->x, node->y);
		if (center->type() == Value::BOOL) {
			node->center = center->toBool();
		}
		break;
	}
	case CIRCLE: {
		const Value r = lookup_radius(c, "d", "r");
		if (r.type() == Value::NUMBER) {
			node->r1 = r.toDouble();
		}
		break;
	}
	case POLYGON: {
		node->points = c.lookup_variable("points");
		node->paths = c.lookup_variable("paths");
		break;
	}
	}

	node->convexity = c.lookup_variable("convexity", true)->toDouble();
	if (node->convexity < 1)
		node->convexity = 1;

	return node;
}

void register_builtin_primitives()
{
	Builtins::init("cube", new PrimitiveModule(CUBE));
	Builtins::init("sphere", new PrimitiveModule(SPHERE));
	Builtins::init("cylinder", new PrimitiveModule(CYLINDER));
	Builtins::init("polyhedron", new PrimitiveModule(POLYHEDRON));
	Builtins::init("square", new PrimitiveModule(SQUARE));
	Builtins::init("circle", new PrimitiveModule(CIRCLE));
	Builtins::init("polygon", new PrimitiveModule(POLYGON));
}

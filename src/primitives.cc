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

	PrimitiveNode *node = nullptr;
	switch (this->type)  {
	case CUBE: {
		ValuePtr size = c.lookup_variable("size");
		ValuePtr center = c.lookup_variable("center");
		double x = 1, y = 1, z = 1;
		size->getDouble(x);
		size->getDouble(y);
		size->getDouble(z);
		size->getVec3(x, y, z);
		bool c = (center->type() == Value::BOOL) ? center->toBool() : false;
		node = new CubeNode(inst, x, y, z, c);
		break;
	}
	case SPHERE: {
		const Value rval = lookup_radius(c, "d", "r");
		double r = rval.type() == Value::NUMBER ? rval.toDouble() : 1;
		node = new SphereNode(inst, r);
		break;
	}
	case CYLINDER: {
		ValuePtr hval = c.lookup_variable("h");
		double h = hval->type() == Value::NUMBER ? hval->toDouble() : 1;

		const Value rval = lookup_radius(c, "d", "r");
		const Value r1val = lookup_radius(c, "d1", "r1");
		const Value r2val = lookup_radius(c, "d2", "r2");
		double r1 = 1, r2 = 1;
		if (rval.type() == Value::NUMBER) {
			r1 = rval.toDouble();
			r2 = rval.toDouble();
		}
		if (r1val.type() == Value::NUMBER) {
			r1 = r1val.toDouble();
		}
		if (r2val.type() == Value::NUMBER) {
			r2 = r2val.toDouble();
		}

		ValuePtr center = c.lookup_variable("center");
		bool c = center->type() == Value::BOOL ? center->toBool() : false;
		node = new CylinderNode(inst, r1, r2, h, c);
		break;
	}
	case POLYHEDRON: {
		ValuePtr points = c.lookup_variable("points");
		ValuePtr faces = c.lookup_variable("faces");
		if (faces->type() == Value::UNDEFINED) {
			// backwards compatible
			faces = c.lookup_variable("triangles", true);
			if (faces->type() != Value::UNDEFINED) {
				printDeprecation("polyhedron(triangles=[]) will be removed in future releases. Use polyhedron(faces=[]) instead.");
			}
		}
		node = new PolyhedronNode(inst, points, faces);
		break;
	}
	case SQUARE: {
		ValuePtr size = c.lookup_variable("size");
		ValuePtr center = c.lookup_variable("center");
		double x = 1, y = 1;
		size->getDouble(x);
		size->getDouble(y);
		size->getVec2(x, y);
		bool c = center->type() == Value::BOOL ? center->toBool() : false;
		node = new SquareNode(inst, x, y, c);
		break;
	}
	case CIRCLE: {
		const Value rval = lookup_radius(c, "d", "r");
		double r = rval.type() == Value::NUMBER ? rval.toDouble() : 1;
		node = new CircleNode(inst, r);
		break;
	}
	case POLYGON: {
		ValuePtr points = c.lookup_variable("points");
		ValuePtr paths = c.lookup_variable("paths");
		node = new PolygonNode(inst, points, paths);
		break;
	}
	}

	int convexity = c.lookup_variable("convexity", true)->toDouble();
	if (convexity < 1) convexity = 1;

	double fn = c.lookup_variable("$fn")->toDouble();
	double fs = c.lookup_variable("$fs")->toDouble();
	double fa = c.lookup_variable("$fa")->toDouble();
	if (fs < F_MINIMUM) {
		PRINTB("WARNING: $fs too small - clamping to %f", F_MINIMUM);
		fs = F_MINIMUM;
	}
	if (fa < F_MINIMUM) {
		PRINTB("WARNING: $fa too small - clamping to %f", F_MINIMUM);
		fa = F_MINIMUM;
	}

	node->convexity = convexity;
	node->fn = fn;
	node->fs = fs;
	node->fa = fa;

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

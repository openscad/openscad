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

#include "transformnode.h"
#include "module.h"
#include "context.h"
#include "polyset.h"
#include "builtin.h"
#include <sstream>
#include <vector>
#include <assert.h>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

enum transform_type_e {
	SCALE,
	ROTATE,
	MIRROR,
	TRANSLATE,
	MULTMATRIX
};

class TransformModule : public AbstractModule
{
public:
	transform_type_e type;
	TransformModule(transform_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

AbstractNode *TransformModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	TransformNode *node = new TransformNode(inst);

	node->matrix = Transform3d::Identity();

	std::vector<std::string> argnames;
	std::vector<Expression*> argexpr;

	switch (this->type) {
	case SCALE:
		argnames += "v";
		break;
	case ROTATE:
		argnames += "a", "v";
		break;
	case MIRROR:
		argnames += "v";
		break;
	case TRANSLATE:
		argnames += "v";
		break;
	case MULTMATRIX:
		argnames += "m";
		break;
	default:
		assert(false);
	}

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	if (this->type == SCALE)
	{
		Vector3d scalevec(1,1,1);
		Value v = c.lookup_variable("v");
		if (!v.getVec3(scalevec[0], scalevec[1], scalevec[2], 1.0)) {
			double num;
			if (v.getDouble(num)) scalevec.setConstant(num);
		}
		node->matrix.scale(scalevec);
	}
	else if (this->type == ROTATE)
	{
		Value val_a = c.lookup_variable("a");
		if (val_a.type() == Value::VECTOR)
		{
			Eigen::AngleAxisd rotx, roty, rotz;
			double a;
			if (val_a.toVector().size() > 0) {
				val_a.toVector()[0].getDouble(a);
				rotx = Eigen::AngleAxisd(a*M_PI/180, Vector3d::UnitX());
			}
			if (val_a.toVector().size() > 1) {
				val_a.toVector()[1].getDouble(a);
				roty = Eigen::AngleAxisd(a*M_PI/180, Vector3d::UnitY());
			}
			if (val_a.toVector().size() > 2) {
				val_a.toVector()[2].getDouble(a);
				rotz = Eigen::AngleAxisd(a*M_PI/180, Vector3d::UnitZ());
			}
			node->matrix.rotate(rotz * roty * rotx);
		}
		else
		{
			Value val_v = c.lookup_variable("v");
			double a = 0;

			val_a.getDouble(a);

			Vector3d axis(0,0,1);
			if (val_v.getVec3(axis[0], axis[1], axis[2])) {
				if (axis.squaredNorm() > 0) axis.normalize();
			}

			if (axis.squaredNorm() > 0) {
				node->matrix = Eigen::AngleAxisd(a*M_PI/180, axis);
			}
		}
	}
	else if (this->type == MIRROR)
	{
		Value val_v = c.lookup_variable("v");
		double x = 1, y = 0, z = 0;
	
		if (val_v.getVec3(x, y, z)) {
			if (x != 0.0 || y != 0.0 || z != 0.0) {
				double sn = 1.0 / sqrt(x*x + y*y + z*z);
				x *= sn, y *= sn, z *= sn;
			}
		}

		if (x != 0.0 || y != 0.0 || z != 0.0)
		{
			Eigen::Matrix4d m;
			m << 1-2*x*x, -2*y*x, -2*z*x, 0,
				-2*x*y, 1-2*y*y, -2*z*y, 0,
				-2*x*z, -2*y*z, 1-2*z*z, 0,
				0, 0, 0, 1;
			node->matrix = m;
		}
	}
	else if (this->type == TRANSLATE)
	{
		Value v = c.lookup_variable("v");
		Vector3d translatevec(0,0,0);
		v.getVec3(translatevec[0], translatevec[1], translatevec[2]);
		node->matrix.translate(translatevec);
	}
	else if (this->type == MULTMATRIX)
	{
		Value v = c.lookup_variable("m");
		if (v.type() == Value::VECTOR) {
			for (int i = 0; i < 16; i++) {
				size_t x = i / 4, y = i % 4;
				if (y < v.toVector().size() && v.toVector()[y].type() == Value::VECTOR && x < v.toVector()[y].toVector().size())
					v.toVector()[y].toVector()[x].getDouble(node->matrix(y, x));
			}
		}
	}

	std::vector<AbstractNode *> evaluatednodes = inst->evaluateChildren();
	node->children.insert(node->children.end(), evaluatednodes.begin(), evaluatednodes.end());

	return node;
}

std::string TransformNode::toString() const
{
	std::stringstream stream;

	stream << "multmatrix([";
	for (int j=0;j<4;j++) {
		stream << "[";
		for (int i=0;i<4;i++) {
			// FIXME: The 0 test is to avoid a leading minus before a single 0 (cosmetics)
			stream << ((this->matrix(j, i)==0)?0:this->matrix(j, i));
			if (i != 3) stream << ", ";
		}
		stream << "]";
		if (j != 3) stream << ", ";
	}
	stream << "])";

	return stream.str();
}

std::string TransformNode::name() const
{
	return "transform";
}

void register_builtin_transform()
{
	Builtins::init("scale", new TransformModule(SCALE));
	Builtins::init("rotate", new TransformModule(ROTATE));
	Builtins::init("mirror", new TransformModule(MIRROR));
	Builtins::init("translate", new TransformModule(TRANSLATE));
	Builtins::init("multmatrix", new TransformModule(MULTMATRIX));
}

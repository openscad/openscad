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
#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "polyset.h"
#include "builtin.h"
#include "value.h"
#include "printutils.h"
#include "degree_trig.h"
#include <sstream>
#include <vector>
#include <assert.h>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

enum class transform_type_e {
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
	AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const override;
};

AbstractNode *TransformModule::instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	auto node = new TransformNode(inst);

	AssignmentList args;

	switch (this->type) {
	case transform_type_e::SCALE:
		args += Assignment("v");
		break;
	case transform_type_e::ROTATE:
		args += Assignment("a"), Assignment("v"), Assignment("from"), Assignment("to");
		break;
	case transform_type_e::MIRROR:
		args += Assignment("v");
		break;
	case transform_type_e::TRANSLATE:
		args += Assignment("v");
		break;
	case transform_type_e::MULTMATRIX:
		args += Assignment("m");
		break;
	default:
		assert(false);
	}

	ContextHandle<Context> c{Context::create<Context>(ctx)};
	c->setVariables(evalctx, args);
	inst->scope.apply(evalctx);

	if (this->type == transform_type_e::SCALE) {
		Vector3d scalevec(1, 1, 1);
		auto v = c->lookup_variable("v");
		if (!v->getVec3(scalevec[0], scalevec[1], scalevec[2], 1.0)) {
			double num;
			if (v->getDouble(num)){
				scalevec.setConstant(num);
			}else{
				PRINTB("WARNING: Unable to convert scale(%s) parameter to a number, a vec3 or vec2 of numbers or a number, %s", v->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
			}
		}
		if(OpenSCAD::rangeCheck){
			if(scalevec[0]==0 || scalevec[1]==0 || scalevec[2]==0 || !std::isfinite(scalevec[0])|| !std::isfinite(scalevec[1])|| !std::isfinite(scalevec[2])){
				PRINTB("WARNING: scale(%s), %s", v->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
			}
		}
		node->matrix.scale(scalevec);
	}
	else if (this->type == transform_type_e::ROTATE) {
		auto val_a = c->lookup_variable("a");
		auto val_v = c->lookup_variable("v");
		auto val_from = c->lookup_variable("from");
		auto val_to = c->lookup_variable("to");
		bool v_supplied = (val_v != ValuePtr::undefined);
		bool from_supplied = (val_from != ValuePtr::undefined);
		bool to_supplied = (val_to != ValuePtr::undefined);
		if (val_a->type() == Value::ValueType::VECTOR) {
			double sx = 0, sy = 0, sz = 0;
			double cx = 1, cy = 1, cz = 1;
			double a = 0.0;
			bool ok = true;
			if (val_a->toVector().size() > 0) {
				ok &= val_a->toVector()[0]->getDouble(a);
				ok &= !std::isinf(a) && !std::isnan(a);
				sx = sin_degrees(a);
				cx = cos_degrees(a);
			}
			if (val_a->toVector().size() > 1) {
				ok &= val_a->toVector()[1]->getDouble(a);
				ok &= !std::isinf(a) && !std::isnan(a);
				sy = sin_degrees(a);
				cy = cos_degrees(a);
			}
			if (val_a->toVector().size() > 2) {
				ok &= val_a->toVector()[2]->getDouble(a);
				ok &= !std::isinf(a) && !std::isnan(a);
				sz = sin_degrees(a);
				cz = cos_degrees(a);
			}
			if (val_a->toVector().size() > 3) {
				ok &= false;
			}
			
			if(ok){
				if(v_supplied){
					PRINTB("WARNING: When parameter a is supplied as vector, v is ignored. rotate(a=%s, v=%s), %s", val_a->toEchoString() % val_v->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}
				if(from_supplied){
					if(to_supplied){
						PRINTB("WARNING: When parameter a is supplied as vector, parameters from and to are ignored. rotate(a=%s, from=%s, to=%s), %s", val_a->toEchoString() % val_from->toEchoString() % val_to->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
					} else {
						PRINTB("WARNING: When parameter a is supplied as vector, parameter from is ignored. rotate(a=%s, from=%s), %s", val_a->toEchoString() % val_from->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
					}
				} else if(to_supplied){
					PRINTB("WARNING: When parameter a is supplied as vector, parameter to is ignored. rotate(a=%s, to=%s), %s", val_a->toEchoString() % val_to->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}
			}else{
				if(v_supplied){
					PRINTB("WARNING: Problem converting rotate(a=%s, v=%s) parameter, %s", val_a->toString() % val_v->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}else{
					PRINTB("WARNING: Problem converting rotate(a=%s) parameter, %s", val_a->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}
			}
			Matrix3d M;
			M <<  cy * cz,  cz * sx * sy - cx * sz,   cx * cz * sy + sx * sz,
			      cy * sz,  cx * cz + sx * sy * sz,  -cz * sx + cx * sy * sz,
			     -sy,       cy * sx,                  cx * cy;
			node->matrix.rotate(M);
		} else {
			double a = 0.0;
			bool aConverted = val_a->getDouble(a);
			aConverted &= !std::isinf(a) && !std::isnan(a);

			Vector3d v(0, 0, 1);
			bool vConverted = val_v->getVec3(v[0], v[1], v[2], 0.0);
			if(from_supplied || to_supplied){
				v[0] = 0;
				v[1] = 0;
				v[2] = 1;
				if (!aConverted) {
					a = 0.0;
				}
			}
			if (from_supplied && to_supplied) {
				Vector3d from_v(0, 0, 1);
				Vector3d to_v(0, 0, 1);
				bool fromConverted = val_from->getVec3(from_v[0], from_v[1], from_v[2], 0.0);
				bool toConverted = val_to->getVec3(to_v[0], to_v[1], to_v[2], 0.0);
				if(!fromConverted && !toConverted){
					PRINTB("WARNING: Problem converting rotate(from=%s, to=%s) parameter, %s", val_from->toEchoString() % val_to->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}else if(!fromConverted){
					PRINTB("WARNING: Problem converting rotate(..., from=%s) parameter, %s", val_from->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}else if(!toConverted){
					PRINTB("WARNING: Problem converting rotate(..., to=%s) parameter, %s", val_to->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}else {
					double x = from_v[1] * to_v[2] - from_v[2] * to_v[1];
					double y = from_v[2] * to_v[0] - from_v[0] * to_v[2];
					double z = from_v[0] * to_v[1] - from_v[1] * to_v[0];
					Vector3d axis(x,y,z);
					double from_len = sqrt(from_v[0]*from_v[0] + from_v[1]*from_v[1] + from_v[2]*from_v[2]);
					double to_len = sqrt(to_v[0]*to_v[0] + to_v[1]*to_v[1] + to_v[2]*to_v[2]);
					double dotprod = from_v[0]*to_v[0] + from_v[1]*to_v[1] + from_v[2]*to_v[2];
					double divisor = from_len * to_len;
					if (divisor == 0.0) {
						PRINTB("WARNING: Invalid zero-length vector in rotate(from=%s, to=%s) parameter, %s", val_from->toEchoString() % val_to->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
					} else {
						double ang = acos(dotprod/divisor) * 180.0 / M_PI;
						node->matrix.rotate(angle_axis_degrees(ang, axis));
					}
				}
			}else if(from_supplied) {
				PRINTB("WARNING: If parameter from is given, then parameter to must also be given in rotate(from=%s), %s", val_from->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
			}else if(to_supplied) {
				PRINTB("WARNING: If parameter to is given, then parameter from must also be given in rotate(to=%s), %s", val_to->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
			}
			node->matrix.rotate(angle_axis_degrees(aConverted ? a : 0, v));
			if(v_supplied && (from_supplied || to_supplied)){
				if (from_supplied) {
					if (to_supplied) {
						PRINTB("WARNING: When parameters from or to are given, parameter v is ignored in rotate(v=%s, from=%s, to=%s) parameter, %s", val_v->toEchoString() % val_from->toEchoString() % val_to->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
					} else {
						PRINTB("WARNING: When parameter from is given, parameter v is ignored in rotate(v=%s, from=%s) parameter, %s", val_v->toEchoString() % val_from->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
					}
				} else {
					PRINTB("WARNING: When parameter to is given, parameter v is ignored in rotate(v=%s, to=%s) parameter, %s", val_v->toEchoString() % val_to->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}
			}else if(v_supplied && ! vConverted){
				if(aConverted){
					PRINTB("WARNING: Problem converting rotate(..., v=%s) parameter, %s", val_v->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}else{
					PRINTB("WARNING: Problem converting rotate(a=%s, v=%s) parameter, %s", val_a->toEchoString() % val_v->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
				}
			}else if(!aConverted && !from_supplied && !to_supplied){
				PRINTB("WARNING: Problem converting rotate(a=%s) parameter, %s", val_a->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
			}
		}
	}
	else if (this->type == transform_type_e::MIRROR) {
		auto val_v = c->lookup_variable("v");
		double x = 1.0, y = 0.0, z = 0.0;
	
		if (val_v->getVec3(x, y, z, 0.0)) {
			if (x != 0.0 || y != 0.0 || z != 0.0) {
				double sn = 1.0 / sqrt(x*x + y*y + z*z);
				x *= sn, y *= sn, z *= sn;
			}
		}else{
			PRINTB("WARNING: Unable to convert mirror(%s) parameter to a vec3 or vec2 of numbers, %s", val_v->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
		}

		if (x != 0.0 || y != 0.0 || z != 0.0)	{
			Matrix4d m;
			m << 1-2*x*x, -2*y*x, -2*z*x, 0,
				-2*x*y, 1-2*y*y, -2*z*y, 0,
				-2*x*z, -2*y*z, 1-2*z*z, 0,
				0, 0, 0, 1;
			node->matrix = m;
		}
	}
	else if (this->type == transform_type_e::TRANSLATE)	{
		auto v = c->lookup_variable("v");
		Vector3d translatevec(0,0,0);
		bool ok = v->getVec3(translatevec[0], translatevec[1], translatevec[2], 0.0);
		ok &= std::isfinite(translatevec[0]) && std::isfinite(translatevec[1]) && std::isfinite(translatevec[2]) ;
		if (ok) {
			node->matrix.translate(translatevec);
		}else{
			PRINTB("WARNING: Unable to convert translate(%s) parameter to a vec3 or vec2 of numbers, %s", v->toEchoString() % inst->location().toRelativeString(ctx->documentPath()));
		}
	}
	else if (this->type == transform_type_e::MULTMATRIX) {
		auto v = c->lookup_variable("m");
		if (v->type() == Value::ValueType::VECTOR) {
			Matrix4d rawmatrix{Matrix4d::Identity()};
			for (int i = 0; i < 16; i++) {
				size_t x = i / 4, y = i % 4;
				if (y < v->toVector().size() && v->toVector()[y]->type() == 
						Value::ValueType::VECTOR && x < v->toVector()[y]->toVector().size())
					v->toVector()[y]->toVector()[x]->getDouble(rawmatrix(y, x));
			}
			double w = rawmatrix(3,3);
			if (w != 1.0) node->matrix = rawmatrix / w;
			else node->matrix = rawmatrix;
		}
	}

	auto instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

std::string TransformNode::toString() const
{
	std::ostringstream stream;

	stream << "multmatrix([";
	for (int j=0;j<4;j++) {
		stream << "[";
		for (int i=0;i<4;i++) {
			Value v(this->matrix(j, i));
			stream << v;
			if (i != 3) stream << ", ";
		}
		stream << "]";
		if (j != 3) stream << ", ";
	}
	stream << "])";

	return stream.str();
}

TransformNode::TransformNode(const ModuleInstantiation *mi) : AbstractNode(mi), matrix(Transform3d::Identity())
{
}

std::string TransformNode::name() const
{
	return "transform";
}

void register_builtin_transform()
{
	Builtins::init("scale", new TransformModule(transform_type_e::SCALE),
				{
					"scale([x, y, z])",
				});

	Builtins::init("rotate", new TransformModule(transform_type_e::ROTATE),
				{
					"rotate([x, y, z])",
				});

	Builtins::init("mirror", new TransformModule(transform_type_e::MIRROR),
				{
					"mirror([x, y, z])",
				});

	Builtins::init("translate", new TransformModule(transform_type_e::TRANSLATE),
				{
					"translate([x, y, z])",
				});

	Builtins::init("multmatrix", new TransformModule(transform_type_e::MULTMATRIX),
				{
					"multmatrix(matrix_4_by_4)",
				});
}

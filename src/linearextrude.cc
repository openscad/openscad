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

#include "linearextrudenode.h"

#include "module.h"
#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "printutils.h"
#include "fileutils.h"
#include "builtin.h"
#include "calc.h"
#include "polyset.h"
#include "handle_dep.h"

#include <cmath>
#include <sstream>
#include "boost-utils.h"
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class LinearExtrudeModule : public AbstractModule
{
public:
	LinearExtrudeModule() { }
	AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const override;
};

AbstractNode *LinearExtrudeModule::instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx) const
{
	auto node = new LinearExtrudeNode(inst, evalctx);

	AssignmentList args{assignment("file"), assignment("layer"), assignment("height"), assignment("origin"), assignment("scale"), assignment("center"), assignment("twist"), assignment("slices")};
	AssignmentList optargs{assignment("convexity")};

	ContextHandle<Context> c{Context::create<Context>(ctx)};
	c->setVariables(evalctx, args, optargs);
	inst->scope.apply(evalctx);

	node->fn = c->lookup_variable("$fn").toDouble();
	node->fs = c->lookup_variable("$fs").toDouble();
	node->fa = c->lookup_variable("$fa").toDouble();

	const auto &file = c->lookup_variable("file");
	const auto &layer = c->lookup_variable("layer", true);
	const auto &height = c->lookup_variable("height", true);
	const auto &convexity = c->lookup_variable("convexity", true);
	const auto &origin = c->lookup_variable("origin", true);
	const auto &scale = c->lookup_variable("scale", true);
	const auto &center = c->lookup_variable("center", true);
	const auto &twist = c->lookup_variable("twist", true);
	const auto &slices = c->lookup_variable("slices", true);

	if (!file.isUndefined() && file.type() == Value::Type::STRING) {
		LOG(message_group::Deprecated,Location::NONE,"","Support for reading files in linear_extrude will be removed in future releases. Use a child import() instead.");
		auto filename = lookup_file(file.toString(), inst->path(), c->documentPath());
		node->filename = filename;
		handle_dep(filename);
	}

	node->height = 100;
	if (height.isDefined()) {
		height.getFiniteDouble(node->height);
	} else {
		// if height not given, and first argument is a number,
		// then assume it should be the height.
		if (evalctx->numArgs() > 0 && evalctx->getArgName(0) == "") {
			auto val = evalctx->getArgValue(0);
			if (val.type() == Value::Type::NUMBER) val.getFiniteDouble(node->height);
		}
	}

	node->layername = layer.isUndefined() ? "" : layer.toString();

	double tmp_convexity = 0.0;
	convexity.getFiniteDouble(tmp_convexity);
	node->convexity = static_cast<int>(tmp_convexity);

	bool originOk = origin.getVec2(node->origin_x, node->origin_y);
	originOk &= std::isfinite(node->origin_x) && std::isfinite(node->origin_y);
	if(origin.isDefined() && !originOk){
		LOG(message_group::Warning,evalctx->loc,ctx->documentPath(),"linear_extrude(..., origin=%1$s) could not be converted",origin.toEchoString());
	}
	node->scale_x = node->scale_y = 1;
	bool scaleOK = scale.getFiniteDouble(node->scale_x);
	scaleOK &= scale.getFiniteDouble(node->scale_y);
	scaleOK |= scale.getVec2(node->scale_x, node->scale_y, true);
	if((origin.isDefined()) && (!scaleOK || !std::isfinite(node->scale_x) || !std::isfinite(node->scale_y))) {
		LOG(message_group::Warning,evalctx->loc,ctx->documentPath(),"linear_extrude(..., scale=%1$s) could not be converted",scale.toEchoString());
	}

	if (center.type() == Value::Type::BOOL)
		node->center = center.toBool();

	if (node->height <= 0) node->height = 0;

	if (node->convexity <= 0)
		node->convexity = 1;

	if (node->scale_x < 0) node->scale_x = 0;
	if (node->scale_y < 0) node->scale_y = 0;

	double slicesVal = 0;
	slices.getFiniteDouble(slicesVal);
	node->slices = static_cast<int>(slicesVal);
	if (node->slices > 0) {
		node->has_slices = true;
	} 

	node->twist = 0.0;
	twist.getFiniteDouble(node->twist);
	if (node->twist != 0.0) {
		node->has_twist = true;
	}

	if (node->filename.empty()) {
		auto instantiatednodes = inst->instantiateChildren(evalctx);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}

	return node;
}

std::string LinearExtrudeNode::toString() const
{
	std::ostringstream stream;

	stream << this->name() << "(";
	if (!this->filename.empty()) { // Ignore deprecated parameters if empty
		fs::path path((std::string)this->filename);
		stream <<
			"file = " << this->filename << ", "
			"layer = " << QuotedString(this->layername) << ", "
			"origin = [" << this->origin_x << ", " << this->origin_y << "], "
			<< "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0) << ", "
			;
	}
	stream <<
		"height = " << std::dec << this->height << ", "
		"center = " << (this->center?"true":"false") << ", "
		"convexity = " << this->convexity;

	if (this->has_twist) {
		stream << ", twist = " << this->twist;
	}
	if (this->has_slices) {
		stream << ", slices = " << this->slices;
	}
	stream << ", scale = [" << this->scale_x << ", " << this->scale_y << "]";
	if (!this->has_slices) {
		stream << ", $fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs;
	}
	stream << ")";
	return stream.str();
}

void register_builtin_dxf_linear_extrude()
{
	Builtins::init("dxf_linear_extrude", new LinearExtrudeModule());

	Builtins::init("linear_extrude", new LinearExtrudeModule(),
				{
					"linear_extrude(number, center = true, convexity = 10, twist, slices = 20, scale = 1.0 [, $fn])",
				});
}

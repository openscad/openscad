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
#include "evalcontext.h"
#include "printutils.h"
#include "fileutils.h"
#include "builtin.h"
#include "PolySetEvaluator.h"
#include "calc.h"
#include "mathc99.h" 

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class LinearExtrudeModule : public AbstractModule
{
public:
	LinearExtrudeModule() { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const;
};

AbstractNode *LinearExtrudeModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	LinearExtrudeNode *node = new LinearExtrudeNode(inst);

	AssignmentList args;
	args += Assignment("file", NULL), Assignment("layer", NULL), Assignment("height", NULL), Assignment("origin", NULL), Assignment("scale", NULL), Assignment("center", NULL), Assignment("twist", NULL), Assignment("slices", NULL);

	Context c(ctx);
	c.setVariables(args, evalctx);

	node->fn = c.lookup_variable("$fn").toDouble();
	node->fs = c.lookup_variable("$fs").toDouble();
	node->fa = c.lookup_variable("$fa").toDouble();

	Value file = c.lookup_variable("file");
	Value layer = c.lookup_variable("layer", true);
	Value height = c.lookup_variable("height", true);
	Value convexity = c.lookup_variable("convexity", true);
	Value origin = c.lookup_variable("origin", true);
	Value scale = c.lookup_variable("scale", true);
	Value center = c.lookup_variable("center", true);
	Value twist = c.lookup_variable("twist", true);
	Value slices = c.lookup_variable("slices", true);

	if (!file.isUndefined() && file.type() == Value::STRING) {
		printDeprecation("DEPRECATED: Support for reading files in linear_extrude will be removed in future releases. Use a child import() instead.");
		node->filename = lookup_file(file.toString(), inst->path(), c.documentPath());
	}

	// if height not given, and first argument is a number,
	// then assume it should be the height.
	if (c.lookup_variable("height").isUndefined() &&
			evalctx->numArgs() > 0 &&
			evalctx->getArgName(0) == "" &&
			evalctx->getArgValue(0).type() == Value::NUMBER) {
		height = evalctx->getArgValue(0);
	}

	node->layername = layer.isUndefined() ? "" : layer.toString();
	node->height = 100;
	height.getDouble(node->height);
	node->convexity = (int)convexity.toDouble();
	origin.getVec2(node->origin_x, node->origin_y);
	node->scale_x = node->scale_y = 1;
	scale.getDouble(node->scale_x);
	scale.getDouble(node->scale_y);
	scale.getVec2(node->scale_x, node->scale_y);

	if (center.type() == Value::BOOL)
		node->center = center.toBool();

	if (node->height <= 0) node->height = 0;

	if (node->convexity <= 0)
		node->convexity = 1;

	if (node->scale_x < 0) node->scale_x = 0;
	if (node->scale_y < 0) node->scale_y = 0;

	if (twist.type() == Value::NUMBER) {
		node->twist = twist.toDouble();
		if (slices.type() == Value::NUMBER) {
			node->slices = (int)slices.toDouble();
		} else {
			node->slices = (int)fmax(2, fabs(Calc::get_fragments_from_r(node->height,
					node->fn, node->fs, node->fa) * node->twist / 360));
		}
		node->has_twist = true;
	}

	if (node->filename.empty()) {
		std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(evalctx);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}

	return node;
}

class PolySet *LinearExtrudeNode::evaluate_polyset(PolySetEvaluator *evaluator) const
{
	if (!evaluator) {
		PRINTB("WARNING: No suitable PolySetEvaluator found for %s module!", this->name());
		return NULL;
	}

	print_messages_push();

	PolySet *ps = evaluator->evaluatePolySet(*this);

	print_messages_pop();

	return ps;
}

std::string LinearExtrudeNode::toString() const
{
	std::stringstream stream;

	stream << this->name() << "(";
	if (!this->filename.empty()) { // Ignore deprecated parameters if empty 
		fs::path path((std::string)this->filename);
		stream <<
			"file = " << this->filename << ", "
			"layer = " << QuotedString(this->layername) << ", "
			"origin = [" << this->origin_x << ", " << this->origin_y << "], "
#ifndef OPENSCAD_TESTING
			// timestamp is needed for caching, but disturbs the test framework
			<< "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0) << ", "
#endif
			;
	}
	stream <<
		"height = " << std::dec << this->height << ", "
		"center = " << (this->center?"true":"false") << ", "
		"convexity = " << this->convexity;
	
	if (this->has_twist) {
		stream << ", twist = " << this->twist << ", slices = " << this->slices;
	}
	stream << ", scale = [" << this->scale_x << ", " << this->scale_y << "]";
	stream << ", $fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs << ")";
	
	return stream.str();
}

void register_builtin_dxf_linear_extrude()
{
	Builtins::init("dxf_linear_extrude", new LinearExtrudeModule());
	Builtins::init("linear_extrude", new LinearExtrudeModule());
}

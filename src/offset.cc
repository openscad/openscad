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

#include "offsetnode.h"

#include "module.h"
#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "printutils.h"
#include "fileutils.h"
#include "builtin.h"
#include "calc.h"
#include "polyset.h"

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class OffsetModule : public AbstractModule
{
public:
	OffsetModule() { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;
};

AbstractNode *OffsetModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	auto node = new OffsetNode(inst);

	AssignmentList args{Assignment("r")};

	Context c(ctx);
	c.setVariables(args, evalctx);
	inst->scope.apply(*evalctx);

	node->fn = c.lookup_variable("$fn")->toDouble();
	node->fs = c.lookup_variable("$fs")->toDouble();
	node->fa = c.lookup_variable("$fa")->toDouble();

	// default with no argument at all is (r = 1, chamfer = false)
	// radius takes precedence if both r and delta are given.
	node->delta = 1;
	node->chamfer = false;
	node->join_type = ClipperLib::jtRound;
	const auto r = c.lookup_variable("r", true);
	const auto delta = c.lookup_variable("delta", true);
	const auto chamfer = c.lookup_variable("chamfer", true);

	if (r->isDefinedAs(Value::ValueType::NUMBER)) {
		r->getDouble(node->delta);
	}
	else if (delta->isDefinedAs(Value::ValueType::NUMBER)) {
		delta->getDouble(node->delta);
		node->join_type = ClipperLib::jtMiter;
		if (chamfer->isDefinedAs(Value::ValueType::BOOL) && chamfer->toBool()) {
			node->chamfer = true;
			node->join_type = ClipperLib::jtSquare;
		}
	}

	auto instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

std::string OffsetNode::toString() const
{
	std::stringstream stream;

	bool isRadius = this->join_type == ClipperLib::jtRound;
	auto var = isRadius ? "(r = " : "(delta = ";

	stream << this->name() << var << std::dec << this->delta;
	if (!isRadius) {
		stream << ", chamfer = " << (this->chamfer ? "true" : "false");
	}
	stream << ", $fn = " << this->fn
				 << ", $fa = " << this->fa
				 << ", $fs = " << this->fs << ")";

	return stream.str();
}

void register_builtin_offset()
{
	Builtins::init("offset", new OffsetModule());
}

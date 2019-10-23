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

#include "extrudenode.h"

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
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class ExtrudeModule : public AbstractModule
{
public:
	ExtrudeModule() : AbstractModule(Feature::ExperimentalExtrude) { }
	AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const override;
};

AbstractNode *ExtrudeModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	auto node = new ExtrudeNode(inst);

	AssignmentList args{};
	AssignmentList optargs{Assignment("convexity")};

	Context c(ctx);
	c.setVariables(evalctx, args, optargs);
	inst->scope.apply(*evalctx);

	auto convexity = c.lookup_variable("convexity", true);
	node->convexity = static_cast<int>(convexity->toDouble());
	if (node->convexity <= 0)
		node->convexity = 1;

	auto instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	return node;
}

std::string ExtrudeNode::toString() const
{
	std::ostringstream stream;

	stream << this->name() << "(";
	stream <<
		"convexity = " << this->convexity;
	return stream.str();
}

void register_builtin_extrude()
{
	Builtins::init("extrude", new ExtrudeModule());
}

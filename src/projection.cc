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

#include "projectionnode.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "printutils.h"
#include "builtin.h"
#include "polyset.h"

#include <assert.h>
#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class ProjectionModule : public AbstractModule
{
public:
	ProjectionModule() { }
	AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const override;
};

AbstractNode *ProjectionModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	auto node = new ProjectionNode(inst);

	AssignmentList args{Assignment("cut")};

	Context c(ctx);
	c.setVariables(args, evalctx);
	inst->scope.apply(*evalctx);

	auto convexity = c.lookup_variable("convexity", true);
	auto cut = c.lookup_variable("cut", true);

	node->convexity = static_cast<int>(convexity->toDouble());

	if (cut->type() == Value::ValueType::BOOL) {
		node->cut_mode = cut->toBool();
	}

	auto instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

std::string ProjectionNode::toString() const
{
	std::stringstream stream;

	stream << "projection(cut = " << (this->cut_mode ? "true" : "false")
				 << ", convexity = " << this->convexity << ")";

	return stream.str();
}

void register_builtin_projection()
{
	Builtins::init("projection", new ProjectionModule());
}

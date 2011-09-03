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

#include "rendernode.h"
#include "module.h"
#include "polyset.h"
#include "context.h"
#include "dxfdata.h"
#include "dxftess.h"
#include "csgterm.h"
#include "builtin.h"
#include "printutils.h"
#include "progress.h"
#include "visitor.h"

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class RenderModule : public AbstractModule
{
public:
	RenderModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

AbstractNode *RenderModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	RenderNode *node = new RenderNode(inst);

	std::vector<std::string> argnames;
	argnames += "convexity";
	std::vector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	Value v = c.lookup_variable("convexity");
	if (v.type == Value::NUMBER)
		node->convexity = (int)v.num;

	foreach (ModuleInstantiation *v, inst->children) {
		AbstractNode *n = v->evaluate(inst->ctx);
		if (n != NULL)
			node->children.push_back(n);
	}

	return node;
}

void register_builtin_render()
{
	builtin_modules["render"] = new RenderModule();
}

std::string RenderNode::toString() const
{
	std::stringstream stream;

	stream << this->name() << "(convexity = " << convexity << ")";

	return stream.str();
}

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
#include "context.h"
#include "printutils.h"
#include "builtin.h"
#include "visitor.h"
#include "PolySetEvaluator.h"

#include <assert.h>
#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class ProjectionModule : public AbstractModule
{
public:
	ProjectionModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

AbstractNode *ProjectionModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	ProjectionNode *node = new ProjectionNode(inst);

	std::vector<std::string> argnames;
	argnames += "cut";
	std::vector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	Value convexity = c.lookup_variable("convexity", true);
	Value cut = c.lookup_variable("cut", true);

	node->convexity = (int)convexity.toDouble();

	if (cut.type() == Value::BOOL)
		node->cut_mode = cut.toBool();

	std::vector<AbstractNode *> evaluatednodes = inst->evaluateChildren();
	node->children.insert(node->children.end(), evaluatednodes.begin(), evaluatednodes.end());

	return node;
}

PolySet *ProjectionNode::evaluate_polyset(PolySetEvaluator *evaluator) const
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

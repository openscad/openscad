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

#include "bendnode.h"

#include "module.h"
#include "evalcontext.h"
#include "printutils.h"
#include "fileutils.h"
#include "builtin.h"
#include "calc.h"
#include "polyset.h"
#include "mathc99.h"

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

class BendModule : public AbstractModule
{
public:
	BendModule() { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const;
};

AbstractNode *BendModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	BendNode *node = new BendNode(inst);

	AssignmentList args;
	args += Assignment("center"), Assignment("fixed"), Assignment("cyl");

	Context c(ctx);
	c.setVariables(args, evalctx);

	Value convexity = c.lookup_variable("convexity", true);
	Value center = c.lookup_variable("center");
	Value fixed = c.lookup_variable("fixed");
	Value cyl = c.lookup_variable("cyl");

	node->convexity = (int)convexity.toDouble();
	center.getVec3(node->center_x, node->center_y, node->center_z);
	fixed.getVec3(node->fixed_x, node->fixed_y, node->fixed_z);
	cyl.getVec3(node->cyl_x, node->cyl_y, node->cyl_z);

	if (node->convexity <= 0)
		node->convexity = 1;

	std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

std::string BendNode::toString() const
{
	std::stringstream stream;

	stream << this->name()
		<< "(center = [ " << center_x << ", " << center_y << ", " << center_z << " ]"
		<< ", fixed = [ " << fixed_x << ", " << fixed_y << ", " << fixed_z << " ]"
		<< ", cyl = [ " << cyl_x << ", " << cyl_y << ", " << cyl_z << " ]"
		<< ", convexity = " << this->convexity << ")";
	
	return stream.str();
}

void register_builtin_bend()
{
	Builtins::init("bend", new BendModule());
}

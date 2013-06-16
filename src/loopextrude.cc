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

#include "loopextrudenode.h"
#include "module.h"
#include "evalcontext.h"
#include "printutils.h"
#include "fileutils.h"
#include "builtin.h"
#include "polyset.h"
#include "visitor.h"
#include "PolySetEvaluator.h"
#include "openscad.h" // get_fragments_from_r()

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class LoopExtrudeModule : public AbstractModule
{
public:
	LoopExtrudeModule() { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const;
};

AbstractNode *LoopExtrudeModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	LoopExtrudeNode *node = new LoopExtrudeNode(inst);

	AssignmentList args;
  args += Assignment("points", NULL), Assignment("vertices", NULL), Assignment("edges", NULL), Assignment("segments", NULL), Assignment("poly", NULL), Assignment("rect", NULL);

	Context c(ctx);
	c.setVariables(args, evalctx);

	node->fn = c.lookup_variable("$fn").toDouble();
	node->fs = c.lookup_variable("$fs").toDouble();
	node->fa = c.lookup_variable("$fa").toDouble();

	//TODO These options can be removed, at the moment they have no use. Do so when we are satisfied with the syntax.
	//node->open = c.lookup_variable("open").toBool();
  //node->outer = c.lookup_variable("outer").toBool();

  node->points     = c.lookup_variable("points");
  node->vertices   = c.lookup_variable("vertices");
  node->edges      = c.lookup_variable("edges");
  node->poly       = c.lookup_variable("poly");
  node->rect       = c.lookup_variable("rect");
  node->segments   = c.lookup_variable("segments");

  node->convexity  = c.lookup_variable("convexity", true).toDouble();
	if (node->convexity < 1) { node->convexity = 1; }

	std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

PolySet *LoopExtrudeNode::evaluate_polyset(PolySetEvaluator *evaluator) const
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

std::string LoopExtrudeNode::toString() const
{
	std::stringstream stream;

	stream <<
	    this->name() <<
      "( points = " << this->points <<
      ", vertices = " << this->vertices <<
      ", edges = " << this->edges <<
      ", poly = " << this->poly <<
      ", rect = " << this->rect <<
      ", segments = " << this->segments <<
      //", open = " << this->open <<
      //" ,outer = " << this->outer <<
		  " ,convexity = " << this->convexity <<
      ", $fn = " << this->fn <<
      ", $fa = " << this->fa <<
      ", $fs = " << this->fs << ")";

	return stream.str();
}

void register_builtin_loop_extrude()
{
	Builtins::init("loop_extrude", new LoopExtrudeModule());
}

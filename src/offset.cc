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

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class OffsetModule : public AbstractModule
{
public:
	OffsetModule() { }
	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const;
};

AbstractNode *OffsetModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	OffsetNode *node = new OffsetNode(inst);

	AssignmentList args;
	args += Assignment("delta");

	Context c(ctx);
	c.setVariables(args, evalctx);

	node->fn = c.lookup_variable("$fn")->toDouble();
	node->fs = c.lookup_variable("$fs")->toDouble();
	node->fa = c.lookup_variable("$fa")->toDouble();

	ValuePtr delta = c.lookup_variable("delta");
	node->delta = 1;
	delta->getDouble(node->delta);
	
	ValuePtr miter_limit = c.lookup_variable("miter_limit", true);
	node->miter_limit = 2;
	miter_limit->getDouble(node->miter_limit);
	
	ValuePtr join_type = c.lookup_variable("join_type", true);
	if (join_type->type() == Value::STRING) {
		std::string jt = join_type->toString();
		if (std::string("bevel") == jt) {
			node->join_type = ClipperLib::jtSquare;
		} else if (std::string("round") == jt) {
			node->join_type = ClipperLib::jtRound;
		} else if (std::string("miter") == jt) {
			node->join_type = ClipperLib::jtMiter;
		} else {
			PRINTB("WARNING: Unknown join_type for offset(): '%s'", jt);
		}
		
		if ((node->join_type != ClipperLib::jtMiter) && !miter_limit->isUndefined()) {
			PRINTB("WARNING: miter_limit is ignored in offset() for join_type: '%s'", jt);
		}
	}
	
	std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(evalctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());

	return node;
}

std::string OffsetNode::toString() const
{
	std::stringstream stream;

	stream  << this->name()
		<< "(delta = " << std::dec << this->delta
		<< ", join_type = \""
			<< (this->join_type == ClipperLib::jtSquare
				? "bevel"
				: this->join_type == ClipperLib::jtRound
					? "round"
					: "miter") << "\"";
	if (this->join_type == ClipperLib::jtMiter) {
		stream << ", miter_limit = " << this->miter_limit;
	}
	stream  << ", $fn = " << this->fn
		<< ", $fa = " << this->fa
		<< ", $fs = " << this->fs << ")";
	
	return stream.str();
}

void register_builtin_offset()
{
	Builtins::init("offset", new OffsetModule());
}

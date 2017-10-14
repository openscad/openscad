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

#include "UserModule.h"
#include "ModuleInstantiation.h"
#include "node.h"
#include "evalcontext.h"
#include "exceptions.h"
#include "stackcheck.h"
#include "modcontext.h"
#include "expression.h"

#include <sstream>

std::deque<std::string> UserModule::module_stack;

AbstractNode *UserModule::instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	if (StackCheck::inst()->check()) {
		throw RecursionException::create("module", inst->name());
		return nullptr;
	}

	// At this point we know that nobody will modify the dependencies of the local scope
	// passed to this instance, so we can populate the context
	inst->scope.apply(*evalctx);
    
	ModuleContext c(ctx, evalctx);
	// set $children first since we might have variables depending on it
	c.set_variable("$children", ValuePtr(double(inst->scope.children.size())));
	module_stack.push_back(inst->name());
	c.set_variable("$parent_modules", ValuePtr(double(module_stack.size())));
	c.initializeModule(*this);
	// FIXME: Set document path to the path of the module
#if 0 && DEBUG
	c.dump(this, inst);
#endif

	AbstractNode *node = new GroupNode(inst);
	std::vector<AbstractNode *> instantiatednodes = this->scope.instantiateChildren(&c);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	module_stack.pop_back();

	return node;
}

std::string UserModule::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	std::string tab;
	if (!name.empty()) {
		dump << indent << "module " << name << "(";
		for (size_t i=0; i < this->definition_arguments.size(); i++) {
			const Assignment &arg = this->definition_arguments[i];
			if (i > 0) dump << ", ";
			dump << arg.name;
			if (arg.expr) dump << " = " << *arg.expr;
		}
		dump << ") {\n";
		tab = "\t";
	}
	dump << scope.dump(indent + tab);
	if (!name.empty()) {
		dump << indent << "}\n";
	}
	return dump.str();
}

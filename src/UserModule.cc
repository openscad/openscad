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
#include "exceptions.h"
#include "stackcheck.h"
#include "modcontext.h"
#include "expression.h"
#include "printutils.h"
#include "compiler_specific.h"
#include <sstream>
#include "boost-utils.h"

std::vector<std::string> StaticModuleNameStack::stack;

static void NOINLINE print_err(std::string name, const Location &loc,const std::shared_ptr<const Context> context){
	LOG(message_group::Error,loc,context->documentRoot(),"Recursion detected calling module '%1$s'",name);
}

AbstractNode* UserModule::instantiate(const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const
{
	if (StackCheck::inst().check()) {
		print_err(inst->name(),loc,context);
		throw RecursionException::create("module", inst->name(),loc);
		return nullptr;
	}

	StaticModuleNameStack name{inst->name()}; // push on static stack, pop at end of method!
	ContextHandle<UserModuleContext> module_context{Context::create<UserModuleContext>(
		defining_context,
		this,
		inst->location(),
		Arguments(inst->arguments, context),
		Children(&inst->scope, context)
	)};
#if 0 && DEBUG
	PRINTDB("UserModuleContext for module %s(%s):\n", this->name % STR(this->parameters));
	PRINTDB("%s", module_context->dump());
#endif

	return this->body.instantiateModules(*module_context, new GroupNode(inst, std::string("module ") + this->name));
}

void UserModule::print(std::ostream &stream, const std::string &indent) const
{
	std::string tab;
	if (!this->name.empty()) {
		stream << indent << "module " << this->name << "(";
		for (size_t i=0; i < this->parameters.size(); ++i) {
			const auto &parameter = this->parameters[i];
			if (i > 0) stream << ", ";
			stream << parameter->getName();
			if (parameter->getExpr()) stream << " = " << *parameter->getExpr();
		}
		stream << ") {\n";
		tab = "\t";
	}
	body.print(stream, indent + tab);
	if (!this->name.empty()) {
		stream << indent << "}\n";
	}
}

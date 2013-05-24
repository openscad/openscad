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

#include "context.h"
#include "evalcontext.h"
#include "expression.h"
#include "function.h"
#include "module.h"
#include "builtin.h"
#include "printutils.h"
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include "boosty.h"

std::vector<const Context*> Context::ctx_stack;

/*!
	Initializes this context. Optionally initializes a context for an external library
*/
Context::Context(const Context *parent)
	: parent(parent)
{
	ctx_stack.push_back(this);
	if (parent) document_path = parent->document_path;
}

Context::~Context()
{
	ctx_stack.pop_back();
}

/*!
	Initialize context from a module argument list and a evaluation context
	which may pass variables which will be preferred over default values.
*/
void Context::setVariables(const AssignmentList &args,
													 const EvalContext *evalctx)
{
	BOOST_FOREACH(const Assignment &arg, args) {
		set_variable(arg.first, arg.second ? arg.second->evaluate(this->parent) : Value());
	}

	if (evalctx) {
		size_t posarg = 0;
		for (size_t i=0; i<evalctx->numArgs(); i++) {
			const std::string &name = evalctx->getArgName(i);
			const Value &val = evalctx->getArgValue(i);
			if (name.empty()) {
				if (posarg < args.size()) this->set_variable(args[posarg++].first, val);
			} else {
				this->set_variable(name, val);
			}
		}
	}
}

void Context::set_variable(const std::string &name, const Value &value)
{
	if (name[0] == '$')
		this->config_variables[name] = value;
	else
		this->variables[name] = value;
}

void Context::set_constant(const std::string &name, const Value &value)
{
	if (this->constants.find(name) != this->constants.end()) {
		PRINTB("WARNING: Attempt to modify constant '%s'.", name);
	}
	else {
		this->constants[name] = value;
	}
}

Value Context::lookup_variable(const std::string &name, bool silent) const
{
	if (name[0] == '$') {
		for (int i = ctx_stack.size()-1; i >= 0; i--) {
			const ValueMap &confvars = ctx_stack[i]->config_variables;
			if (confvars.find(name) != confvars.end())
				return confvars.find(name)->second;
		}
		return Value();
	}
	if (!this->parent && this->constants.find(name) != this->constants.end())
		return this->constants.find(name)->second;
	if (this->variables.find(name) != this->variables.end())
		return this->variables.find(name)->second;
	if (this->parent)
		return this->parent->lookup_variable(name, silent);
	if (!silent)
		PRINTB("WARNING: Ignoring unknown variable '%s'.", name);
	return Value();
}

Value Context::evaluate_function(const std::string &name, const EvalContext *evalctx) const
{
	if (this->parent) return this->parent->evaluate_function(name, evalctx);
	PRINTB("WARNING: Ignoring unknown function '%s'.", name);
	return Value();
}

AbstractNode *Context::instantiate_module(const ModuleInstantiation &inst, const EvalContext *evalctx) const
{
	if (this->parent) return this->parent->instantiate_module(inst, evalctx);
	PRINTB("WARNING: Ignoring unknown module '%s'.", inst.name());
	return NULL;
}

/*!
	Returns the absolute path to the given filename, unless it's empty.
 */
std::string Context::getAbsolutePath(const std::string &filename) const
{
	if (!filename.empty() && !boosty::is_absolute(fs::path(filename))) {
		return boosty::absolute(fs::path(this->document_path) / filename).string();
	}
	else {
		return filename;
	}
}

#ifdef DEBUG
void Context::dump(const AbstractModule *mod, const ModuleInstantiation *inst)
{
	if (inst) 
		PRINTB("ModuleContext %p (%p) for %s inst (%p)", this % this->parent % inst->name() % inst);
	else 
		PRINTB("Context: %p (%p)", this % this->parent);
	PRINTB("  document path: %s", this->document_path);
	if (mod) {
		const Module *m = dynamic_cast<const Module*>(mod);
		if (m) {
			PRINT("  module args:");
			BOOST_FOREACH(const Assignment &arg, m->definition_arguments) {
				PRINTB("    %s = %s", arg.first % variables[arg.first]);
			}
		}
	}
	typedef std::pair<std::string, Value> ValueMapType;
	PRINT("  vars:");
  BOOST_FOREACH(const ValueMapType &v, constants) {
	  PRINTB("    %s = %s", v.first % v.second);
	}		
  BOOST_FOREACH(const ValueMapType &v, variables) {
	  PRINTB("    %s = %s", v.first % v.second);
	}		
  BOOST_FOREACH(const ValueMapType &v, config_variables) {
	  PRINTB("    %s = %s", v.first % v.second);
	}		

}
#endif

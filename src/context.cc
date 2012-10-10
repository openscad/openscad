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
Context::Context(const Context *parent, const Module *library)
	: parent(parent), inst_p(NULL)
{
	if (parent) recursioncount = parent->recursioncount;
	ctx_stack.push_back(this);
	if (parent) document_path = parent->document_path;
	if (library) {
		// FIXME: Don't access module members directly
		this->functions_p = &library->functions;
		this->modules_p = &library->modules;
		this->usedlibs_p = &library->usedlibs;
		for (size_t j = 0; j < library->assignments_var.size(); j++) {
			this->set_variable(library->assignments_var[j], 
												 library->assignments_expr[j]->evaluate(this));
		}
	}
	else {
		functions_p = NULL;
		modules_p = NULL;
		usedlibs_p = NULL;
	}
}

Context::~Context()
{
	ctx_stack.pop_back();
}

/*!
	Initialize context from argument lists (function call/module instantiation)
 */
void Context::args(const std::vector<std::string> &argnames, 
									 const std::vector<Expression*> &argexpr,
									 const std::vector<std::string> &call_argnames, 
									 const std::vector<Value> &call_argvalues)
{
	for (size_t i=0; i<argnames.size(); i++) {
		set_variable(argnames[i], i < argexpr.size() && argexpr[i] ? 
								 argexpr[i]->evaluate(this->parent) : Value());
	}

	size_t posarg = 0;
	for (size_t i=0; i<call_argnames.size(); i++) {
		if (call_argnames[i].empty()) {
			if (posarg < argnames.size())
				set_variable(argnames[posarg++], call_argvalues[i]);
		} else {
			set_variable(call_argnames[i], call_argvalues[i]);
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

class RecursionGuard
{
public:
	RecursionGuard(const Context &c, const std::string &name) : c(c), name(name) { c.recursioncount[name]++; }
	~RecursionGuard() { if (--c.recursioncount[name] == 0) c.recursioncount.erase(name); }
	bool recursion_detected() const { return (c.recursioncount[name] > 100); }
private:
	const Context &c;
	const std::string &name;
};

Value Context::evaluate_function(const std::string &name, 
																 const std::vector<std::string> &argnames, 
																 const std::vector<Value> &argvalues) const
{
	RecursionGuard g(*this, name);
	if (g.recursion_detected()) { 
		PRINTB("Recursion detected calling function '%s'", name);
		return Value();
	}
	if (this->functions_p && this->functions_p->find(name) != this->functions_p->end())
		return this->functions_p->find(name)->second->evaluate(this, argnames, argvalues);
	if (this->usedlibs_p) {
		BOOST_FOREACH(const ModuleContainer::value_type &m, *this->usedlibs_p) {
			if (m.second->functions.find(name) != m.second->functions.end()) {
				Context ctx(this->parent, m.second);
				return m.second->functions[name]->evaluate(&ctx, argnames, argvalues);
			}
		}
	}
	if (this->parent) return this->parent->evaluate_function(name, argnames, argvalues);
	PRINTB("WARNING: Ignoring unknown function '%s'.", name);
	return Value();
}

AbstractNode *Context::evaluate_module(const ModuleInstantiation &inst) const
{
	if (this->modules_p && this->modules_p->find(inst.name()) != this->modules_p->end()) {
		AbstractModule *m = this->modules_p->find(inst.name())->second;
		std::string replacement = Builtins::instance()->isDeprecated(inst.name());
		if (!replacement.empty()) {
			PRINTB("DEPRECATED: The %s() module will be removed in future releases. Use %s() instead.", inst.name() % replacement);
		}
		return m->evaluate(this, &inst);
	}
	if (this->usedlibs_p) {
		BOOST_FOREACH(const ModuleContainer::value_type &m, *this->usedlibs_p) {
			assert(m.second);
			if (m.second->modules.find(inst.name()) != m.second->modules.end()) {
				Context ctx(this->parent, m.second);
				return m.second->modules[inst.name()]->evaluate(&ctx, &inst);
			}
		}
	}
	if (this->parent) return this->parent->evaluate_module(inst);
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

void register_builtin(Context &ctx)
{
	ctx.functions_p = &Builtins::instance()->functions();
	ctx.modules_p = &Builtins::instance()->modules();
	ctx.set_variable("$fn", Value(0.0));
	ctx.set_variable("$fs", Value(2.0));
	ctx.set_variable("$fa", Value(12.0));
	ctx.set_variable("$t", Value(0.0));
	
	Value::VectorType zero3;
	zero3.push_back(Value(0.0));
	zero3.push_back(Value(0.0));
	zero3.push_back(Value(0.0));
	Value zero3val(zero3);
	ctx.set_variable("$vpt", zero3val);
	ctx.set_variable("$vpr", zero3val);

	ctx.set_constant("PI",Value(M_PI));
}

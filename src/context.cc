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
#include "printutils.h"
#include <QFileInfo>
#include <QDir>
#include <boost/foreach.hpp>

Context::Context(const Context *parent)
{
	this->parent = parent;
	functions_p = NULL;
	modules_p = NULL;
	usedlibs_p = NULL;
	inst_p = NULL;
	if (parent) document_path = parent->document_path;
	ctx_stack.push_back(this);
}

Context::~Context()
{
	ctx_stack.pop_back();
}

void Context::args(const std::vector<std::string> &argnames, const std::vector<Expression*> &argexpr,
		const std::vector<std::string> &call_argnames, const std::vector<Value> &call_argvalues)
{
	for (size_t i=0; i<argnames.size(); i++) {
		set_variable(argnames[i], i < argexpr.size() && argexpr[i] ? argexpr[i]->evaluate(this->parent) : Value());
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

std::vector<const Context*> Context::ctx_stack;

void Context::set_variable(const std::string &name, Value value)
{
	if (name[0] == '$')
		config_variables[name] = value;
	else
		variables[name] = value;
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
	if (!parent && constants.find(name) != constants.end())
		return constants.find(name)->second;
	if (variables.find(name) != variables.end())
		return variables.find(name)->second;
	if (parent)
		return parent->lookup_variable(name, silent);
	if (!silent)
		PRINTF("WARNING: Ignoring unknown variable '%s'.", name.c_str());
	return Value();
}

void Context::set_constant(const std::string &name, Value value)
{
	if (constants.count(name))
		PRINTF("WARNING: Attempt to modify constant '%s'.",name.c_str());
	else
		constants[name] = value;
}

Value Context::evaluate_function(const std::string &name, const std::vector<std::string> &argnames, const std::vector<Value> &argvalues) const
{
	if (functions_p && functions_p->find(name) != functions_p->end())
		return functions_p->find(name)->second->evaluate(this, argnames, argvalues);
	if (usedlibs_p) {
		BOOST_FOREACH(const ModuleContainer::value_type &m, *usedlibs_p) {
			if (m.second->functions.count(name)) {
				Module *lib = m.second;
				Context ctx(parent);
				ctx.functions_p = &lib->functions;
				ctx.modules_p = &lib->modules;
				ctx.usedlibs_p = &lib->usedlibs;
				for (size_t j = 0; j < lib->assignments_var.size(); j++) {
					ctx.set_variable(lib->assignments_var[j], lib->assignments_expr[j]->evaluate(&ctx));
				}
				return m.second->functions[name]->evaluate(&ctx, argnames, argvalues);
			}
		}
	}
	if (parent)
		return parent->evaluate_function(name, argnames, argvalues);
	PRINTF("WARNING: Ignoring unkown function '%s'.", name.c_str());
	return Value();
}

AbstractNode *Context::evaluate_module(const ModuleInstantiation *inst) const
{
	if (modules_p && modules_p->find(inst->modname) != modules_p->end())
		return modules_p->find(inst->modname)->second->evaluate(this, inst);
	if (usedlibs_p) {
		BOOST_FOREACH(const ModuleContainer::value_type &m, *usedlibs_p) {
			if (m.second->modules.count(inst->modname)) {
				Module *lib = m.second;
				Context ctx(parent);
				ctx.functions_p = &lib->functions;
				ctx.modules_p = &lib->modules;
				ctx.usedlibs_p = &lib->usedlibs;
				for (size_t j = 0; j < lib->assignments_var.size(); j++) {
					ctx.set_variable(lib->assignments_var[j], lib->assignments_expr[j]->evaluate(&ctx));
				}
				return m.second->modules[inst->modname]->evaluate(&ctx, inst);
			}
		}
	}
	if (parent)
		return parent->evaluate_module(inst);
	PRINTF("WARNING: Ignoring unkown module '%s'.", inst->modname.c_str());
	return NULL;
}

/*!
	Returns the absolute path to the given filename, unless it's empty.
 */
std::string Context::get_absolute_path(const std::string &filename) const
{
	if (!filename.empty()) {
		return QFileInfo(QDir(QString::fromStdString(this->document_path)), 
										 QString::fromStdString(filename)).absoluteFilePath().toStdString();
	}
	else {
		return filename;
	}
}


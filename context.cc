/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include "openscad.h"

Context::Context(const Context *parent)
{
	this->parent = parent;
	functions_p = NULL;
	modules_p = NULL;
	ctx_stack.append(this);
}

Context::~Context()
{
	ctx_stack.pop_back();
}

void Context::args(const QVector<QString> &argnames, const QVector<Expression*> &argexpr,
		const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues)
{
	for (int i=0; i<argnames.size(); i++) {
		set_variable(argnames[i], i < argexpr.size() && argexpr[i] ? argexpr[i]->evaluate(this->parent) : Value());
	}

	int posarg = 0;
	for (int i=0; i<call_argnames.size(); i++) {
		if (call_argnames[i].isEmpty()) {
			if (posarg < argnames.size())
				set_variable(argnames[posarg++], call_argvalues[i]);
		} else {
			set_variable(call_argnames[i], call_argvalues[i]);
		}
	}
}

QVector<const Context*> Context::ctx_stack;

void Context::set_variable(QString name, Value value)
{
	if (name.startsWith("$"))
		config_variables[name] = value;
	else
		variables[name] = value;
}

Value Context::lookup_variable(QString name, bool silent) const
{
	if (name.startsWith("$")) {
		for (int i = ctx_stack.size()-1; i >= 0; i--) {
			if (ctx_stack[i]->config_variables.contains(name))
				return ctx_stack[i]->config_variables[name];
		}
		return Value();
	}
	if (variables.contains(name))
		return variables[name];
	if (parent)
		return parent->lookup_variable(name, silent);
	if (!silent)
		PRINTA("WARNING: Ignoring unkown variable '%1'.", name);
	return Value();
}

Value Context::evaluate_function(QString name, const QVector<QString> &argnames, const QVector<Value> &argvalues) const
{
	if (functions_p && functions_p->contains(name))
		return functions_p->value(name)->evaluate(this, argnames, argvalues);
	if (parent)
		return parent->evaluate_function(name, argnames, argvalues);
	PRINTA("WARNING: Ignoring unkown function '%1'.", name);
	return Value();
}

AbstractNode *Context::evaluate_module(const ModuleInstanciation *inst) const
{
	if (modules_p && modules_p->contains(inst->modname))
		return modules_p->value(inst->modname)->evaluate(this, inst);
	if (parent)
		return parent->evaluate_module(inst);
	PRINTA("WARNING: Ignoring unkown module '%1'.", inst->modname);
	return NULL;
}


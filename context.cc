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

void Context::args(const QVector<QString> &argnames, const QVector<Expression*> &argexpr,
		const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues)
{
	for (int i=0; i<argnames.size(); i++) {
		variables[argnames[i]] = i < argexpr.size() && argexpr[i] ? argexpr[i]->evaluate(this->parent) : Value();
	}

	int posarg = 0;
	for (int i=0; i<call_argnames.size(); i++) {
		if (call_argnames[i].isEmpty()) {
			variables[argnames[posarg++]] = call_argvalues[i];
		} else {
			variables[call_argnames[i]] = call_argvalues[i];
		}
	}
}

Value Context::lookup_variable(QString name) const
{
	if (variables.contains(name))
		return variables[name];
	if (parent)
		return parent->lookup_variable(name);
	return Value();
}

Value Context::evaluate_function(QString name, const QVector<QString> &argnames, const QVector<Value> &argvalues) const
{
	if (functions_p->contains(name))
		return functions_p->value(name)->evaluate(this, argnames, argvalues);
	if (parent)
		return parent->evaluate_function(name, argnames, argvalues);
	return Value();
}

AbstractNode *Context::evaluate_module(QString name, const QVector<QString> &argnames, const QVector<Value> &argvalues, const QVector<AbstractNode*> child_nodes) const
{
	if (modules_p->contains(name))
		return modules_p->value(name)->evaluate(this, argnames, argvalues, child_nodes);
	if (parent)
		return parent->evaluate_module(name, argnames, argvalues, child_nodes);
	return NULL;
}


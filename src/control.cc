/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
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

#include "module.h"
#include "node.h"
#include "context.h"
#include "builtin.h"
#include "printutils.h"

enum control_type_e {
	CHILD,
	ECHO,
	ASSIGN,
	FOR,
	INT_FOR,
	IF
};

class ControlModule : public AbstractModule
{
public:
	control_type_e type;
	ControlModule(control_type_e type) : type(type) { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

void for_eval(AbstractNode *node, int l, const QVector<QString> &call_argnames, const QVector<Value> &call_argvalues, const QVector<ModuleInstantiation*> arg_children, const Context *arg_context)
{
	if (call_argnames.size() > l) {
		QString it_name = call_argnames[l];
		Value it_values = call_argvalues[l];
		Context c(arg_context);
		if (it_values.type == Value::RANGE) {
			double range_begin = it_values.range_begin;
			double range_end = it_values.range_end;
			double range_step = it_values.range_step;
			if (range_end < range_begin) {
				double t = range_begin;
				range_begin = range_end;
				range_end = t;
			}
			if (range_step > 0 && (range_begin-range_end)/range_step < 10000) {
				for (double i = range_begin; i <= range_end; i += range_step) {
					c.set_variable(it_name, Value(i));
					for_eval(node, l+1, call_argnames, call_argvalues, arg_children, &c);
				}
			}
		}
		else if (it_values.type == Value::VECTOR) {
			for (int i = 0; i < it_values.vec.size(); i++) {
				c.set_variable(it_name, *it_values.vec[i]);
				for_eval(node, l+1, call_argnames, call_argvalues, arg_children, &c);
			}
		}
		else {
			for_eval(node, l+1, call_argnames, call_argvalues, arg_children, &c);
		}
	} else {
		foreach (ModuleInstantiation *v, arg_children) {
			AbstractNode *n = v->evaluate(arg_context);
			if (n != NULL)
				node->children.append(n);
		}
	}
}

AbstractNode *ControlModule::evaluate(const Context*, const ModuleInstantiation *inst) const
{
	if (type == CHILD)
	{
		int n = 0;
		if (inst->argvalues.size() > 0) {
			double v;
			if (inst->argvalues[0].getnum(v))
				n = v;
		}
		for (int i = Context::ctx_stack.size()-1; i >= 0; i--) {
			const Context *c = Context::ctx_stack[i];
			if (c->inst_p) {
				if (n < c->inst_p->children.size())
					return c->inst_p->children[n]->evaluate(c->inst_p->ctx);
				return NULL;
			}
			c = c->parent;
		}
		return NULL;
	}

	AbstractNode *node;

	if (type == INT_FOR)
		node = new AbstractIntersectionNode(inst);
	else
		node = new AbstractNode(inst);

	if (type == ECHO)
	{
		QString msg = QString("ECHO: ");
		for (int i = 0; i < inst->argnames.size(); i++) {
			if (i > 0)
				msg += QString(", ");
			if (!inst->argnames[i].isEmpty())
				msg += inst->argnames[i] + QString(" = ");
			msg += inst->argvalues[i].dump();
		}
		PRINT(msg);
	}

	if (type == ASSIGN)
	{
		Context c(inst->ctx);
		for (int i = 0; i < inst->argnames.size(); i++) {
			if (!inst->argnames[i].isEmpty())
				c.set_variable(inst->argnames[i], inst->argvalues[i]);
		}
		foreach (ModuleInstantiation *v, inst->children) {
			AbstractNode *n = v->evaluate(&c);
			if (n != NULL)
				node->children.append(n);
		}
	}

	if (type == FOR || type == INT_FOR)
	{
		for_eval(node, 0, inst->argnames, inst->argvalues, inst->children, inst->ctx);
	}

	if (type == IF)
	{
		if (inst->argvalues.size() > 0 && inst->argvalues[0].type == Value::BOOL && inst->argvalues[0].b)
			foreach (ModuleInstantiation *v, inst->children) {
				AbstractNode *n = v->evaluate(inst->ctx);
				if (n != NULL)
					node->children.append(n);
			}
	}

	return node;
}

void register_builtin_control()
{
	builtin_modules["child"] = new ControlModule(CHILD);
	builtin_modules["echo"] = new ControlModule(ECHO);
	builtin_modules["assign"] = new ControlModule(ASSIGN);
	builtin_modules["for"] = new ControlModule(FOR);
	builtin_modules["intersection_for"] = new ControlModule(INT_FOR);
	builtin_modules["if"] = new ControlModule(IF);
}


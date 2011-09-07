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

#include "module.h"
#include "node.h"
#include "context.h"
#include "builtin.h"
#include "printutils.h"
#include <sstream>

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

void for_eval(AbstractNode &node, const ModuleInstantiation &inst, size_t l, 
							const std::vector<std::string> &call_argnames, 
							const std::vector<Value> &call_argvalues, 
							const Context *arg_context)
{
	if (call_argnames.size() > l) {
		const std::string &it_name = call_argnames[l];
		const Value &it_values = call_argvalues[l];
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
					for_eval(node, inst, l+1, call_argnames, call_argvalues, &c);
				}
			}
		}
		else if (it_values.type == Value::VECTOR) {
			for (size_t i = 0; i < it_values.vec.size(); i++) {
				c.set_variable(it_name, *it_values.vec[i]);
				for_eval(node, inst, l+1, call_argnames, call_argvalues, &c);
			}
		}
		else if (it_values.type != Value::UNDEFINED) {
			c.set_variable(it_name, it_values);
			for_eval(node, inst, l+1, call_argnames, call_argvalues, &c);
		}
	} else if (l > 0) {
		std::vector<AbstractNode *> evaluatednodes = inst.evaluateChildren(arg_context);
		node.children.insert(node.children.end(), evaluatednodes.begin(), evaluatednodes.end());
	}
}

AbstractNode *ControlModule::evaluate(const Context*, const ModuleInstantiation *inst) const
{
	if (type == CHILD)
	{
		size_t n = 0;
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
		std::stringstream msg;
		msg << "ECHO: ";
		for (size_t i = 0; i < inst->argnames.size(); i++) {
			if (i > 0) msg << ", ";
			if (!inst->argnames[i].empty()) msg << inst->argnames[i] << " = ";
			msg << inst->argvalues[i];
		}
		PRINTF("%s", msg.str().c_str());
	}

	if (type == ASSIGN)
	{
		Context c(inst->ctx);
		for (size_t i = 0; i < inst->argnames.size(); i++) {
			if (!inst->argnames[i].empty())
				c.set_variable(inst->argnames[i], inst->argvalues[i]);
		}
		std::vector<AbstractNode *> evaluatednodes = inst->evaluateChildren(&c);
		node->children.insert(node->children.end(), evaluatednodes.begin(), evaluatednodes.end());
	}

	if (type == FOR || type == INT_FOR)
	{
		for_eval(*node, *inst, 0, inst->argnames, inst->argvalues, inst->ctx);
	}

	if (type == IF)
	{
		const IfElseModuleInstantiation *ifelse = dynamic_cast<const IfElseModuleInstantiation*>(inst);
		if (ifelse->argvalues.size() > 0 && ifelse->argvalues[0].type == Value::BOOL && ifelse->argvalues[0].b) {
			std::vector<AbstractNode *> evaluatednodes = ifelse->evaluateChildren();
			node->children.insert(node->children.end(), evaluatednodes.begin(), evaluatednodes.end());
		}
		else {
			std::vector<AbstractNode *> evaluatednodes = ifelse->evaluateElseChildren();
			node->children.insert(node->children.end(), evaluatednodes.begin(), evaluatednodes.end());
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


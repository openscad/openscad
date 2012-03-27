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
		if (it_values.type() == Value::RANGE) {
			Value::RangeType range = it_values.toRange();
			if (range.end < range.begin) {
				double t = range.begin;
				range.begin = range.end;
				range.end = t;
			}
			if (range.step > 0 && (range.begin-range.end)/range.step < 10000) {
				for (double i = range.begin; i <= range.end; i += range.step) {
					c.set_variable(it_name, Value(i));
					for_eval(node, inst, l+1, call_argnames, call_argvalues, &c);
				}
			}
		}
		else if (it_values.type() == Value::VECTOR) {
			for (size_t i = 0; i < it_values.toVector().size(); i++) {
				c.set_variable(it_name, it_values.toVector()[i]);
				for_eval(node, inst, l+1, call_argnames, call_argvalues, &c);
			}
		}
		else if (it_values.type() != Value::UNDEFINED) {
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
	AbstractNode *node = NULL;

	if (type == CHILD)
	{
		size_t n = 0;
		if (inst->argvalues.size() > 0) {
			double v;
			if (inst->argvalues[0].getDouble(v))
				n = v;
		}
		for (int i = Context::ctx_stack.size()-1; i >= 0; i--) {
			const Context *c = Context::ctx_stack[i];
			if (c->inst_p) {
				if (n < c->inst_p->children.size()) {
					node = c->inst_p->children[n]->evaluate(c->inst_p->ctx);
					// FIXME: We'd like to inherit any tags from the ModuleInstantiation
					// given as parameter to this method. However, the instantition which belongs
					// to the returned node cannot be changed. This causes the test
					// features/child-background.scad to fail.
				}
				return node;
			}
			c = c->parent;
		}
		return NULL;
	}

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
		PRINTB("%s", msg.str());
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
		if (ifelse->argvalues.size() > 0 && ifelse->argvalues[0].toBool()) {
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
	Builtins::init("child", new ControlModule(CHILD));
	Builtins::init("echo", new ControlModule(ECHO));
	Builtins::init("assign", new ControlModule(ASSIGN));
	Builtins::init("for", new ControlModule(FOR));
	Builtins::init("intersection_for", new ControlModule(INT_FOR));
	Builtins::init("if", new ControlModule(IF));
}

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
#include "evalcontext.h"
#include "modcontext.h"
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
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const;
};

void for_eval(AbstractNode &node, const ModuleInstantiation &inst, size_t l, 
							const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->eval_arguments.size() > l) {
		const std::string &it_name = evalctx->eval_arguments[l].first;
		const Value &it_values = evalctx->eval_arguments[l].second;
		Context c(ctx);
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
					for_eval(node, inst, l+1, &c, evalctx);
				}
			}
		}
		else if (it_values.type() == Value::VECTOR) {
			for (size_t i = 0; i < it_values.toVector().size(); i++) {
				c.set_variable(it_name, it_values.toVector()[i]);
				for_eval(node, inst, l+1, &c, evalctx);
			}
		}
		else if (it_values.type() != Value::UNDEFINED) {
			c.set_variable(it_name, it_values);
			for_eval(node, inst, l+1, &c, evalctx);
		}
	} else if (l > 0) {
		std::vector<AbstractNode *> evaluatednodes = inst.evaluateChildren(ctx);
		node.children.insert(node.children.end(), evaluatednodes.begin(), evaluatednodes.end());
	}
}

AbstractNode *ControlModule::evaluate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	AbstractNode *node = NULL;

	if (type == CHILD)
	{
		int n = 0;
		if (evalctx->eval_arguments.size() > 0) {
			double v;
			if (evalctx->eval_arguments[0].second.getDouble(v)) {
				n = trunc(v);
				if (n < 0) {
					PRINTB("WARNING: Negative child index (%d) not allowed", n);
					return NULL; // Disallow negative child indices
				}
			}
		}

		// Find the last custom module invocation, which will contain
		// an eval context with the children of the module invokation
		const Context *tmpc = evalctx;
		while (tmpc->parent) {
			const ModuleContext *filectx = dynamic_cast<const ModuleContext*>(tmpc->parent);
			if (filectx) {
        // This will trigger if trying to invoke child from the root of any file
        // assert(filectx->evalctx);

				if (filectx->evalctx) {
					if (n < filectx->evalctx->children.size()) {
						node = filectx->evalctx->children[n]->evaluate_instance(filectx->evalctx);
					}
					else {
						// How to deal with negative objects in this case?
            // (e.g. first child of difference is invalid)
						PRINTB("WARNING: Child index (%d) out of bounds (%d children)", 
									 n % filectx->evalctx->children.size());
					}
				}
				return node;
			}
			tmpc = tmpc->parent;
		}
		return node;
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
			if (!evalctx->eval_arguments[i].first.empty()) msg << evalctx->eval_arguments[i].first << " = ";
			msg << evalctx->eval_arguments[i].second;
		}
		PRINTB("%s", msg.str());
	}

	if (type == ASSIGN)
	{
		Context c(evalctx);
		for (size_t i = 0; i < evalctx->eval_arguments.size(); i++) {
			if (!evalctx->eval_arguments[i].first.empty())
				c.set_variable(evalctx->eval_arguments[i].first, evalctx->eval_arguments[i].second);
		}
		std::vector<AbstractNode *> evaluatednodes = inst->evaluateChildren(&c);
		node->children.insert(node->children.end(), evaluatednodes.begin(), evaluatednodes.end());
	}

	if (type == FOR || type == INT_FOR)
	{
		for_eval(*node, *inst, 0, evalctx, evalctx);
	}

	if (type == IF)
	{
		const IfElseModuleInstantiation *ifelse = dynamic_cast<const IfElseModuleInstantiation*>(inst);
		if (evalctx->eval_arguments.size() > 0 && evalctx->eval_arguments[0].second.toBool()) {
			std::vector<AbstractNode *> evaluatednodes = ifelse->evaluateChildren(evalctx);
			node->children.insert(node->children.end(), evaluatednodes.begin(), evaluatednodes.end());
		}
		else {
			std::vector<AbstractNode *> evaluatednodes = ifelse->evaluateElseChildren(evalctx);
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

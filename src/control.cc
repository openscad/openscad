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
#include "ModuleInstantiation.h"
#include "node.h"
#include "arguments.h"
#include "evalcontext.h"
#include "modcontext.h"
#include "expression.h"
#include "builtin.h"
#include "printutils.h"
#include <cstdint>
#include "boost-utils.h"

static const std::shared_ptr<EvalContext> getLastModuleCtx(const std::shared_ptr<EvalContext> evalctx)
{
	// Find the last custom module invocation, which will contain
	// an eval context with the children of the module invocation
	std::shared_ptr<Context> ctx = evalctx;
	while (ctx->getParent()) {
		const ModuleContext *modulectx = dynamic_cast<const ModuleContext*>(ctx->getParent().get());
		if (modulectx) {
			// This will trigger if trying to invoke child from the root of any file
			// assert(filectx->evalctx);
			if (modulectx->evalctx) {
				return modulectx->evalctx;
			}
			return nullptr;
		}
		ctx = ctx->getParent();
	}
	return nullptr;
}

static AbstractNode* getChild(int n, const std::shared_ptr<EvalContext> &modulectx)
{
	if (n < 0) {
		LOG(message_group::Warning,Location::NONE,"","Negative children index (%1$d) not allowed",n);
		return nullptr; // Disallow negative child indices
	}
	if (n >= static_cast<int>(modulectx->numChildren())) {
		// How to deal with negative objects in this case?
		// (e.g. first child of difference is invalid)
		LOG(message_group::Warning,Location::NONE,"","Children index (%1$d) out of bounds (%2$d children)",n,modulectx->numChildren());
		return nullptr;
	}
	// OK
	return modulectx->getChild(n)->evaluate(modulectx);
}

static AbstractNode* getChild(const Value &value, const std::shared_ptr<EvalContext> &modulectx)
{
	if (value.type() != Value::Type::NUMBER) {
		// Invalid argument
		// (e.g. first child of difference is invalid)
		LOG(message_group::Warning,Location::NONE,"","Bad parameter type (%1$s) for children, only accept: empty, number, vector, range.",value.toString());
		return nullptr;
	}
	double v;
	if (!value.getDouble(v)) {
		LOG(message_group::Warning,Location::NONE,"","Bad parameter type (%1$s) for children, only accept: empty, number, vector, range.",value.toString());
		return nullptr;
	}
	return getChild(static_cast<int>(trunc(v)), modulectx);
}

static AbstractNode* builtin_child(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	LOG(message_group::Deprecated,Location::NONE,"","child() will be removed in future releases. Use children() instead.");
	int n = 0;
	if (evalctx->numArgs() > 0) {
		double v;
		if (evalctx->getArgValue(0).getDouble(v)) {
			n = trunc(v);
			if (n < 0) {
				LOG(message_group::Warning,evalctx->loc,evalctx->documentRoot(),
					"Negative child index (%1$d) not allowed",n);
				return nullptr; // Disallow negative child indices
			}
		}
	}

	// Find the last custom module invocation, which will contain
	// an eval context with the children of the module invocation
	const std::shared_ptr<EvalContext> modulectx = getLastModuleCtx(evalctx);
	if (!modulectx) {
		return nullptr;
	}
	// This will trigger if trying to invoke child from the root of any file
	if (n < (int)modulectx->numChildren()) {
		return modulectx->getChild(n)->evaluate(modulectx);
	}
	// How to deal with negative objects in this case?
	// (e.g. first child of difference is invalid)
	LOG(message_group::Warning,evalctx->loc,evalctx->documentRoot(),
		"Child index (%1$d) out of bounds (%2$d children)",n,modulectx->numChildren());
	return nullptr;
}

static AbstractNode* builtin_children(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	const std::shared_ptr<EvalContext> modulectx = getLastModuleCtx(evalctx);
	if (!modulectx) {
		return nullptr;
	}
	
	// This will trigger if trying to invoke child from the root of any file
	// assert(filectx->evalctx);
	if (evalctx->numArgs()<=0) {
		// no arguments => all children
		AbstractNode* node;
		if (Feature::ExperimentalLazyUnion.is_enabled()) node = new ListNode(inst);
		else node = new GroupNode(inst);

		for (int n = 0; n < (int)modulectx->numChildren(); ++n) {
			AbstractNode* childnode = modulectx->getChild(n)->evaluate(modulectx);
			if (childnode==nullptr) continue; // error
			node->children.push_back(childnode);
		}
		return node;
	}
	
	// one (or more ignored) argument
	Value value = evalctx->getArgValue(0);
	if (value.type() == Value::Type::NUMBER) {
		return getChild(value, modulectx);
	}
	else if (value.type() == Value::Type::VECTOR) {
		AbstractNode* node;
		if (Feature::ExperimentalLazyUnion.is_enabled()) node = new ListNode(inst);
		else node = new GroupNode(inst);
		for(const auto& val : value.toVector()) {
			AbstractNode* childnode = getChild(val, modulectx);
			if (childnode==nullptr) continue; // error
			node->children.push_back(childnode);
		}
		return node;
	}
	else if (value.type() == Value::Type::RANGE) {
		const RangeType &range = value.toRange();
		uint32_t steps = range.numValues();
		if (steps >= RangeType::MAX_RANGE_STEPS) {
			LOG(message_group::Warning,evalctx->loc,evalctx->documentRoot(),
				"Bad range parameter for children: too many elements (%1$lu)",steps);
			return nullptr;
		}
		AbstractNode* node;
		if (Feature::ExperimentalLazyUnion.is_enabled()) node = new ListNode(inst);
		else node = new GroupNode(inst);
		for (double d : range) {
			AbstractNode* childnode = getChild(static_cast<int>(trunc(d)), modulectx); // with error cases
			if (childnode==nullptr) continue; // error
			node->children.push_back(childnode);
		}
		return node;
	}
	else {
		// Invalid argument
		// (e.g. first child of difference is invalid)
		LOG(message_group::Warning,evalctx->loc,evalctx->documentRoot(), "Bad parameter type (%1$s) for children, only accept: empty, number, vector, range",value.toEchoString());
		return nullptr;
	}
}

static AbstractNode* builtin_echo(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode *node;
	if (Feature::ExperimentalLazyUnion.is_enabled()) node = new ListNode(inst);
	else node = new GroupNode(inst);
	LOG(message_group::Echo,Location::NONE,"",STR(*evalctx));
	ContextHandle<Context> c{Context::create<Context>(evalctx)};
	inst->scope.apply(c.ctx);
	node->children = inst->instantiateChildren(c.ctx);
	// echo without child geometries should not count as valid CSGNode
	if (node->children.empty()) return nullptr;
	return node;
}

static AbstractNode* builtin_assert(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode *node;
	if (Feature::ExperimentalLazyUnion.is_enabled()) node = new ListNode(inst);
	else node = new GroupNode(inst);
	ContextHandle<Context> c{Context::create<Context>(evalctx)};
	evaluate_assert(c.ctx, evalctx);
	inst->scope.apply(c.ctx);
	node->children = inst->instantiateChildren(c.ctx);
	// assert without child geometries should not count as valid CSGNode
	if (node->children.empty()) return nullptr;
	return node;
}

static AbstractNode* builtin_let(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode *node = nullptr;
	if (Feature::ExperimentalLazyUnion.is_enabled()) node = new ListNode(inst);
	else node = new GroupNode(inst);
	ContextHandle<Context> c{Context::create<Context>(evalctx)};

	evalctx->assignTo(c.ctx);

	inst->scope.apply(c.ctx);
	std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(c.ctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	return node;
}

static AbstractNode* builtin_assign(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode *node = new GroupNode(inst);
	// We create a new context to avoid arguments from influencing each other
	// -> parallel evaluation. This is to be backwards compatible.
	ContextHandle<Context> c{Context::create<Context>(evalctx)};
	for (size_t i = 0; i < evalctx->numArgs(); ++i) {
		if (!evalctx->getArgName(i).empty())
			c->set_variable(evalctx->getArgName(i), evalctx->getArgValue(i));
	}
	// Let any local variables override the arguments
	inst->scope.apply(c.ctx);
	std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(c.ctx);
	node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	return node;
}

static void for_eval(AbstractNode &node, const ModuleInstantiation &inst, size_t l,
							const std::shared_ptr<Context> ctx, const std::shared_ptr<EvalContext> evalctx)
{
	if (evalctx->numArgs() > l) {
		const std::string &it_name = evalctx->getArgName(l);
		Value it_values = evalctx->getArgValue(l, ctx);
		ContextHandle<Context> c{Context::create<Context>(ctx)};
		if (it_values.type() == Value::Type::RANGE) {
			const RangeType &range = it_values.toRange();
			uint32_t steps = range.numValues();
			if (steps >= RangeType::MAX_RANGE_STEPS) {
				LOG(message_group::Warning,inst.location(),evalctx->documentRoot(),
					"Bad range parameter in for statement: too many elements (%1$lu)",steps);
			} else {
				for (double d : range) {
					c->set_variable(it_name, d);
					for_eval(node, inst, l+1, c.ctx, evalctx);
				}
			}
		}
		else if (it_values.type() == Value::Type::VECTOR) {
			for (const auto &el : it_values.toVector()) {
				c->set_variable(it_name, el.clone());
				for_eval(node, inst, l+1, c.ctx, evalctx);
			}
		}
		else if (it_values.type() == Value::Type::STRING) {
			for(auto ch : it_values.toStrUtf8Wrapper()) {
				c->set_variable(it_name, Value(std::move(ch)));
				for_eval(node, inst, l+1, c.ctx, evalctx);
			}
		}
		else if (it_values.type() != Value::Type::UNDEFINED) {
			c->set_variable(it_name, std::move(it_values));
			for_eval(node, inst, l+1, c.ctx, evalctx);
		}
	} else if (l > 0) {
		// At this point, the for loop variables have been set and we can initialize
		// the local scope (as they may depend on the for loop variables
		ContextHandle<Context> c{Context::create<Context>(ctx)};
		for (const auto &assignment : inst.scope.assignments) {
			c->set_variable(assignment->getName(), assignment->getExpr()->evaluate(c.ctx));
		}

		std::vector<AbstractNode *> instantiatednodes = inst.instantiateChildren(c.ctx);
		node.children.insert(node.children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
}

static AbstractNode* builtin_for(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode *node;
	if (Feature::ExperimentalLazyUnion.is_enabled()) node = new ListNode(inst);
	else node = new GroupNode(inst);
	for_eval(*node, *inst, 0, evalctx, evalctx);
	return node;
}

static AbstractNode* builtin_intersection_for(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode *node = new AbstractIntersectionNode(inst);
	for_eval(*node, *inst, 0, evalctx, evalctx);
	return node;
}

static AbstractNode* builtin_if(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode *node;
	if (Feature::ExperimentalLazyUnion.is_enabled()) node = new ListNode(inst);
	else node = new GroupNode(inst);
	const IfElseModuleInstantiation *ifelse = dynamic_cast<const IfElseModuleInstantiation*>(inst);
	if (evalctx->numArgs() > 0 && evalctx->getArgValue(0).toBool()) {
		inst->scope.apply(evalctx);
		std::vector<AbstractNode *> instantiatednodes = ifelse->instantiateChildren(evalctx);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
	else {
		LocalScope* else_scope = ifelse->getElseScope();
		if (else_scope) {
			else_scope->apply(evalctx);
			std::vector<AbstractNode *> instantiatednodes = ifelse->instantiateElseChildren(evalctx);
			node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
		} else {
			// "if" with failed condition, and no "else" should not count as valid CSGNode
			return nullptr;
		}
	}
	return node;
}

void register_builtin_control()
{
	Builtins::init("assign", new BuiltinModule(builtin_assign));
	Builtins::init("child", new BuiltinModule(builtin_child));

	Builtins::init("children", new BuiltinModule(builtin_children),
				{
					"children()",
					"children(number)",
					"children([start : step : end])",
					"children([start : end])",
					"children([vector])",
				});

	Builtins::init("echo", new BuiltinModule(builtin_echo),
				{
					"echo(arg, ...)",
				});

	Builtins::init("assert", new BuiltinModule(builtin_assert),
				{
					"assert(boolean)",
					"assert(boolean, string)",
				});

	Builtins::init("for", new BuiltinModule(builtin_for),
				{
					"for([start : increment : end])",
					"for([start : end])",
					"for([vector])",
				});

	Builtins::init("let", new BuiltinModule(builtin_let),
				{
					"let(arg, ...) expression",
				});

	Builtins::init("intersection_for", new BuiltinModule(builtin_intersection_for),
				{
					"intersection_for([start : increment : end])",
					"intersection_for([start : end])",
					"intersection_for([vector])",
				});

	Builtins::init("if", new BuiltinModule(builtin_if),
				{
					"if(boolean)",
				});
}

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

static AbstractNode* lazyUnionNode(const ModuleInstantiation *inst)
{
	if (Feature::ExperimentalLazyUnion.is_enabled()) {
		return new ListNode(inst);
	} else {
		return new GroupNode(inst);
	}
}

static boost::optional<size_t> validChildIndex(int n, const Children* children)
{
	if (n < 0 || n >= static_cast<int>(children->size())) {
		LOG(message_group::Warning,Location::NONE,"","Children index (%1$d) out of bounds (%2$d children)",n,children->size());
		return boost::none;
	}
	return size_t(n);
}

static boost::optional<size_t> validChildIndex(const Value &value, const Children* children)
{
	if (value.type() != Value::Type::NUMBER) {
		LOG(message_group::Warning,Location::NONE,"","Bad parameter type (%1$s) for children, only accept: empty, number, vector, range.",value.toString());
		return boost::none;
	}
	return validChildIndex(static_cast<int>(trunc(value.toDouble())), children);
}

static AbstractNode* builtin_child(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	LOG(message_group::Deprecated,Location::NONE,"","child() will be removed in future releases. Use children() instead.");
	
	Arguments arguments{evalctx->getArgs(), evalctx->get_shared_ptr()};
	const Children* children = evalctx->user_module_children();
	if (!children) {
		// child() called outside any user module
		return nullptr;
	}
	
	boost::optional<size_t> index;
	if (arguments.size() == 0) {
		index = validChildIndex(0, children);
	} else {
		index = validChildIndex(arguments[0].value, children);
	}
	if (!index) {
		return nullptr;
	}
	return children->instantiate(*index);
}

static AbstractNode* builtin_children(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	Arguments arguments{evalctx->getArgs(), evalctx->get_shared_ptr()};
	const Children* children = evalctx->user_module_children();
	if (!children) {
		// children() called outside any user module
		return nullptr;
	}
	
	if (arguments.size() == 0) {
		// no arguments => all children
		return children->instantiate(lazyUnionNode(inst));
	}
	
	// one (or more ignored) argument
	if (arguments[0]->type() == Value::Type::NUMBER) {
		auto index = validChildIndex(arguments[0].value, children);
		if (!index) {
			return nullptr;
		}
		return children->instantiate(*index);
	}
	else if (arguments[0]->type() == Value::Type::VECTOR) {
		std::vector<size_t> indices;
		for (const auto& val : arguments[0]->toVector()) {
			auto index = validChildIndex(val, children);
			if (index) {
				indices.push_back(*index);
			}
		}
		return children->instantiate(lazyUnionNode(inst), indices);
	}
	else if (arguments[0]->type() == Value::Type::RANGE) {
		const RangeType &range = arguments[0]->toRange();
		uint32_t steps = range.numValues();
		if (steps >= RangeType::MAX_RANGE_STEPS) {
			LOG(message_group::Warning,inst->location(),arguments.documentRoot(),
				"Bad range parameter for children: too many elements (%1$lu)",steps);
			return nullptr;
		}
		std::vector<size_t> indices;
		for (double d : range) {
			auto index = validChildIndex(static_cast<int>(trunc(d)), children);
			if (index) {
				indices.push_back(*index);
			}
		}
		return children->instantiate(lazyUnionNode(inst), indices);
	}
	else {
		// Invalid argument
		LOG(message_group::Warning,inst->location(),arguments.documentRoot(), "Bad parameter type (%1$s) for children, only accept: empty, number, vector, range",arguments[0]->toEchoString());
		return nullptr;
	}
}

static AbstractNode* builtin_echo(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	LOG(message_group::Echo,Location::NONE,"",STR(*evalctx));
	
	AbstractNode* node = Children(&inst->scope, evalctx->get_shared_ptr()).instantiate(lazyUnionNode(inst));
	// echo without child geometries should not count as valid CSGNode
	if (node->children.empty()) {
		delete node;
		return nullptr;
	}
	return node;
}

static AbstractNode* builtin_assert(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	evaluate_assert(evalctx, evalctx);
	
	AbstractNode* node = Children(&inst->scope, evalctx->get_shared_ptr()).instantiate(lazyUnionNode(inst));
	// assert without child geometries should not count as valid CSGNode
	if (node->children.empty()) {
		delete node;
		return nullptr;
	}
	return node;
}

static AbstractNode* builtin_let(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	ContextHandle<Context> c{Context::create<Context>(evalctx)};
	evalctx->assignTo(c.ctx);
	
	return Children(&inst->scope, c.ctx).instantiate(lazyUnionNode(inst));
}

static AbstractNode* builtin_assign(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	// We create a new context to avoid arguments from influencing each other
	// -> parallel evaluation. This is to be backwards compatible.
	ContextHandle<Context> c{Context::create<Context>(evalctx)};
	for (size_t i = 0; i < evalctx->numArgs(); ++i) {
		if (!evalctx->getArgName(i).empty())
			c->set_variable(evalctx->getArgName(i), evalctx->getArgValue(i));
	}
	
	return Children(&inst->scope, c.ctx).instantiate(lazyUnionNode(inst));
}

static void for_eval(AbstractNode *node, const ModuleInstantiation &inst, size_t l,
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
		Children(&inst.scope, c.ctx).instantiate(node);
	}
}

static AbstractNode* builtin_for(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode* node = lazyUnionNode(inst);
	for_eval(node, *inst, 0, evalctx, evalctx);
	return node;
}

static AbstractNode* builtin_intersection_for(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode *node = new AbstractIntersectionNode(inst);
	for_eval(node, *inst, 0, evalctx, evalctx);
	return node;
}

static AbstractNode* builtin_if(const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx)
{
	AbstractNode* node = lazyUnionNode(inst);
	const IfElseModuleInstantiation *ifelse = dynamic_cast<const IfElseModuleInstantiation*>(inst);
	if (evalctx->numArgs() > 0 && evalctx->getArgValue(0).toBool()) {
		Children(&inst->scope, evalctx->get_shared_ptr()).instantiate(node);
	}
	else {
		if (ifelse->getElseScope()) {
			Children(ifelse->getElseScope(), evalctx->get_shared_ptr()).instantiate(node);
		} else {
			// "if" with failed condition, and no "else" should not count as valid CSGNode
			delete node;
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

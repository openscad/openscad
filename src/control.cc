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
#include "evalcontext.h"
#include "modcontext.h"
#include "expression.h"
#include "builtin.h"
#include "printutils.h"
#include <cstdint>
#include <sstream>

class ControlModule : public AbstractModule
{
public: // types
	enum class Type {
		CHILD,
		CHILDREN,
		ECHO,
		ASSERT,
		ASSIGN,
		FOR,
		LET,
		INT_FOR,
		IF
	};
public: // methods
	ControlModule(Type type) : type(type) { }

	ControlModule(Type type, const Feature &feature) : AbstractModule(feature), type(type) { }

	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, EvalContext *evalctx) const;

	static void for_eval(AbstractNode &node, const ModuleInstantiation &inst, size_t l,
											 const Context *ctx, const EvalContext *evalctx);

	static const EvalContext *getLastModuleCtx(const EvalContext *evalctx);

	static AbstractNode *getChild(const ValuePtr &value, const EvalContext *modulectx);

private: // data
	Type type;

}; // class ControlModule

void ControlModule::for_eval(AbstractNode &node, const ModuleInstantiation &inst, size_t l,
														 const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() > l) {
		const std::string &it_name = evalctx->getArgName(l);
		ValuePtr it_values = evalctx->getArgValue(l, ctx);
		Context c(ctx);
		if (it_values->type() == Value::ValueType::RANGE) {
			RangeType range = it_values->toRange();
			uint32_t steps = range.numValues();
			if (steps >= 10000) {
				PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu).", steps);
			}
			else {
				for (RangeType::iterator it = range.begin(); it != range.end(); it++) {
					c.set_variable(it_name, ValuePtr(*it));
					for_eval(node, inst, l + 1, &c, evalctx);
				}
			}
		}
		else if (it_values->type() == Value::ValueType::VECTOR) {
			for (size_t i = 0; i < it_values->toVector().size(); i++) {
				c.set_variable(it_name, it_values->toVector()[i]);
				for_eval(node, inst, l + 1, &c, evalctx);
			}
		}
		else if (it_values->type() != Value::ValueType::UNDEFINED) {
			c.set_variable(it_name, it_values);
			for_eval(node, inst, l + 1, &c, evalctx);
		}
	}
	else if (l > 0) {
		// At this point, the for loop variables have been set and we can initialize
		// the local scope (as they may depend on the for loop variables
		Context c(ctx);
		for (const auto &ass : inst.scope.assignments) {
			c.set_variable(ass.name, ass.expr->evaluate(&c));
		}

		std::vector<AbstractNode *> instantiatednodes = inst.instantiateChildren(&c);
		node.children.insert(node.children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
}

const EvalContext *ControlModule::getLastModuleCtx(const EvalContext *evalctx)
{
	// Find the last custom module invocation, which will contain
	// an eval context with the children of the module invokation
	const Context *tmpc = evalctx;
	while (tmpc->getParent()) {
		const ModuleContext *modulectx = dynamic_cast<const ModuleContext *>(tmpc->getParent());
		if (modulectx) {
			// This will trigger if trying to invoke child from the root of any file
			// assert(filectx->evalctx);
			if (modulectx->evalctx) {
				return modulectx->evalctx;
			}
			return nullptr;
		}
		tmpc = tmpc->getParent();
	}
	return nullptr;
}

// static
AbstractNode *ControlModule::getChild(const ValuePtr &value, const EvalContext *modulectx)
{
	if (value->type() != Value::ValueType::NUMBER) {
		// Invalid parameter
		// (e.g. first child of difference is invalid)
		PRINTB("WARNING: Bad parameter type (%s) for children, only accept: empty, number, vector, range.", value->toString());
		return nullptr;
	}
	double v;
	if (!value->getDouble(v)) {
		PRINTB("WARNING: Bad parameter type (%s) for children, only accept: empty, number, vector, range.", value->toString());
		return nullptr;
	}

	int n = static_cast<int>(trunc(v));
	if (n < 0) {
		PRINTB("WARNING: Negative children index (%d) not allowed", n);
		return nullptr; // Disallow negative child indices
	}
	if (n >= static_cast<int>(modulectx->numChildren())) {
		// How to deal with negative objects in this case?
		// (e.g. first child of difference is invalid)
		PRINTB("WARNING: Children index (%d) out of bounds (%d children)"
					 , n % modulectx->numChildren());
		return nullptr;
	}
	// OK
	return modulectx->getChild(n)->evaluate(modulectx);
}

AbstractNode *ControlModule::instantiate(const Context * /*ctx*/, const ModuleInstantiation *inst, EvalContext *evalctx) const
{
	AbstractNode *node = nullptr;

	switch (this->type) {
	case Type::CHILD: {
		printDeprecation("child() will be removed in future releases. Use children() instead.");
		int n = 0;
		if (evalctx->numArgs() > 0) {
			double v;
			if (evalctx->getArgValue(0)->getDouble(v)) {
				n = trunc(v);
				if (n < 0) {
					PRINTB("WARNING: Negative child index (%d) not allowed", n);
					return nullptr; // Disallow negative child indices
				}
			}
		}

		// Find the last custom module invocation, which will contain
		// an eval context with the children of the module invokation
		const EvalContext *modulectx = getLastModuleCtx(evalctx);
		if (modulectx == nullptr) {
			return nullptr;
		}
		// This will trigger if trying to invoke child from the root of any file
		if (n < (int)modulectx->numChildren()) {
			node = modulectx->getChild(n)->evaluate(modulectx);
		}
		else {
			// How to deal with negative objects in this case?
			// (e.g. first child of difference is invalid)
			PRINTB("WARNING: Child index (%d) out of bounds (%d children)",
						 n % modulectx->numChildren());
		}
		return node;
	}
										break;

	case Type::CHILDREN: {
		const EvalContext *modulectx = getLastModuleCtx(evalctx);
		if (modulectx == nullptr) {
			return nullptr;
		}
		// This will trigger if trying to invoke child from the root of any file
		// assert(filectx->evalctx);
		if (evalctx->numArgs() <= 0) {
			// no parameters => all children
			AbstractNode *node = new GroupNode(inst);
			for (int n = 0; n < (int)modulectx->numChildren(); ++n) {
				AbstractNode *childnode = modulectx->getChild(n)->evaluate(modulectx);
				if (childnode == nullptr) continue; // error
				node->children.push_back(childnode);
			}
			return node;
		}
		else if (evalctx->numArgs() > 0) {
			// one (or more ignored) parameter
			ValuePtr value = evalctx->getArgValue(0);
			if (value->type() == Value::ValueType::NUMBER) {
				return getChild(value, modulectx);
			}
			else if (value->type() == Value::ValueType::VECTOR) {
				AbstractNode *node = new GroupNode(inst);
				const Value::VectorType &vect = value->toVector();
				for (const auto &vectvalue : vect) {
					AbstractNode *childnode = getChild(vectvalue, modulectx);
					if (childnode == nullptr) continue; // error
					node->children.push_back(childnode);
				}
				return node;
			}
			else if (value->type() == Value::ValueType::RANGE) {
				RangeType range = value->toRange();
				uint32_t steps = range.numValues();
				if (steps >= 10000) {
					PRINTB("WARNING: Bad range parameter for children: too many elements (%lu).", steps);
					return nullptr;
				}
				AbstractNode *node = new GroupNode(inst);
				for (RangeType::iterator it = range.begin(); it != range.end(); it++) {
					AbstractNode *childnode = getChild(ValuePtr(*it), modulectx); // with error cases
					if (childnode == nullptr) continue; // error
					node->children.push_back(childnode);
				}
				return node;
			}
			else {
				// Invalid parameter
				// (e.g. first child of difference is invalid)
				PRINTB("WARNING: Bad parameter type (%s) for children, only accept: empty, number, vector, range.", value->toString());
				return nullptr;
			}
		}
		return nullptr;
	}
											 break;

	case Type::ECHO: {
		node = new GroupNode(inst);
		std::stringstream msg;
		msg << "ECHO: " << *evalctx;
		PRINTB("%s", msg.str());
	}
									 break;

	case Type::ASSERT: {
		node = new GroupNode(inst);

		Context c(evalctx);
		evaluate_assert(c, evalctx, inst->location());
		inst->scope.apply(c);
		node->children = inst->instantiateChildren(&c);
	}
										 break;

	case Type::LET: {
		node = new GroupNode(inst);
		Context c(evalctx);

		evalctx->assignTo(c);

		inst->scope.apply(c);
		std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(&c);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
									break;

	case Type::ASSIGN: {
		node = new GroupNode(inst);
		// We create a new context to avoid parameters from influencing each other
		// -> parallel evaluation. This is to be backwards compatible.
		Context c(evalctx);
		for (size_t i = 0; i < evalctx->numArgs(); i++) {
			if (!evalctx->getArgName(i).empty()) c.set_variable(evalctx->getArgName(i), evalctx->getArgValue(i));
		}
		// Let any local variables override the parameters
		inst->scope.apply(c);
		std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(&c);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
										 break;

	case Type::FOR:
		node = new GroupNode(inst);
		for_eval(*node, *inst, 0, evalctx, evalctx);
		break;

	case Type::INT_FOR:
		node = new AbstractIntersectionNode(inst);
		for_eval(*node, *inst, 0, evalctx, evalctx);
		break;

	case Type::IF: {
		node = new GroupNode(inst);
		const IfElseModuleInstantiation *ifelse = dynamic_cast<const IfElseModuleInstantiation *>(inst);
		if (evalctx->numArgs() > 0 && evalctx->getArgValue(0)->toBool()) {
			inst->scope.apply(*evalctx);
			std::vector<AbstractNode *> instantiatednodes = ifelse->instantiateChildren(evalctx);
			node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
		}
		else {
			ifelse->else_scope.apply(*evalctx);
			std::vector<AbstractNode *> instantiatednodes = ifelse->instantiateElseChildren(evalctx);
			node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
		}
	}
								 break;
	}
	return node;
}

void register_builtin_control()
{
	Builtins::init("child", new ControlModule(ControlModule::Type::CHILD));
	Builtins::init("children", new ControlModule(ControlModule::Type::CHILDREN));
	Builtins::init("echo", new ControlModule(ControlModule::Type::ECHO));
	Builtins::init("assert", new ControlModule(ControlModule::Type::ASSERT, Feature::ExperimentalAssertExpression));
	Builtins::init("assign", new ControlModule(ControlModule::Type::ASSIGN));
	Builtins::init("for", new ControlModule(ControlModule::Type::FOR));
	Builtins::init("let", new ControlModule(ControlModule::Type::LET));
	Builtins::init("intersection_for", new ControlModule(ControlModule::Type::INT_FOR));
	Builtins::init("if", new ControlModule(ControlModule::Type::IF));
}

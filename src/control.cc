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

#include <boost/foreach.hpp>
#include "module.h"
#include "node.h"
#include "evalcontext.h"
#include "modcontext.h"
#include "builtin.h"
#include "printutils.h"
#include <sstream>
#include "mathc99.h"


#define foreach BOOST_FOREACH


class ControlModule : public AbstractModule
{
public: // types
	enum Type {
		CHILD,
		CHILDREN,
		ECHO,
		ASSIGN,
		FOR,
		INT_FOR,
		IF
    };
public: // methods
	ControlModule(Type type)
		: type(type)
	{ }

	virtual AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, const EvalContext *evalctx) const;

	static void for_eval(AbstractNode &node, const ModuleInstantiation &inst, size_t l, 
						 const Context *ctx, const EvalContext *evalctx);

	static const EvalContext* getLastModuleCtx(const EvalContext *evalctx);
	
	static AbstractNode* getChild(const Value& value, const EvalContext* modulectx);

private: // data
	Type type;

}; // class ControlModule

void ControlModule::for_eval(AbstractNode &node, const ModuleInstantiation &inst, size_t l, 
							const Context *ctx, const EvalContext *evalctx)
{
	if (evalctx->numArgs() > l) {
		const std::string &it_name = evalctx->getArgName(l);
		const Value &it_values = evalctx->getArgValue(l, ctx);
		Context c(ctx);
		if (it_values.type() == Value::RANGE) {
			Value::RangeType range = it_values.toRange();
                        boost::uint32_t steps = range.nbsteps();
                        if (steps >= 10000) {
                                PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu).", steps);
                        } else {
                            for (Value::RangeType::iterator it = range.begin();it != range.end();it++) {
                                c.set_variable(it_name, Value(*it));
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
		std::vector<AbstractNode *> instantiatednodes = inst.instantiateChildren(ctx);
		node.children.insert(node.children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}
}

const EvalContext* ControlModule::getLastModuleCtx(const EvalContext *evalctx)
{
	// Find the last custom module invocation, which will contain
	// an eval context with the children of the module invokation
	const Context *tmpc = evalctx;
	while (tmpc->parent) {
		const ModuleContext *modulectx = dynamic_cast<const ModuleContext*>(tmpc->parent);
		if (modulectx) {
			// This will trigger if trying to invoke child from the root of any file
			// assert(filectx->evalctx);
			if (modulectx->evalctx) {
				return modulectx->evalctx;
			}
			return NULL;
		}
		tmpc = tmpc->parent;
	}
	return NULL;
}

// static
AbstractNode* ControlModule::getChild(const Value& value, const EvalContext* modulectx)
{
	if (value.type()!=Value::NUMBER) {
		// Invalid parameter
		// (e.g. first child of difference is invalid)
		PRINTB("WARNING: Bad parameter type (%s) for children, only accept: empty, number, vector, range.", value.toString());
		return NULL;
	}
	double v;
	if (!value.getDouble(v)) {
		PRINTB("WARNING: Bad parameter type (%s) for children, only accept: empty, number, vector, range.", value.toString());
		return NULL;
	}
		
	int n = trunc(v);
	if (n < 0) {
		PRINTB("WARNING: Negative children index (%d) not allowed", n);
		return NULL; // Disallow negative child indices
	}
	if (n>=(int)modulectx->numChildren()) {
		// How to deal with negative objects in this case?
		// (e.g. first child of difference is invalid)
		PRINTB("WARNING: Children index (%d) out of bounds (%d children)"
			, n % modulectx->numChildren());
		return NULL;
	}
	// OK
	return modulectx->getChild(n)->evaluate(modulectx);
}

AbstractNode *ControlModule::instantiate(const Context* /*ctx*/, const ModuleInstantiation *inst, const EvalContext *evalctx) const
{
	AbstractNode *node = NULL;

	if (type == CHILD)
	{
		printDeprecation("DEPRECATED: child() will be removed in future releases. Use children() instead.");
		int n = 0;
		if (evalctx->numArgs() > 0) {
			double v;
			if (evalctx->getArgValue(0).getDouble(v)) {
				n = trunc(v);
				if (n < 0) {
					PRINTB("WARNING: Negative child index (%d) not allowed", n);
					return NULL; // Disallow negative child indices
				}
			}
		}

		// Find the last custom module invocation, which will contain
		// an eval context with the children of the module invokation
		const EvalContext *modulectx = getLastModuleCtx(evalctx);
		if (modulectx==NULL) {
			return NULL;
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

	if (type == CHILDREN)
	{
		const EvalContext *modulectx = getLastModuleCtx(evalctx);
		if (modulectx==NULL) {
			return NULL;
		}
		// This will trigger if trying to invoke child from the root of any file
		// assert(filectx->evalctx);
		if (evalctx->numArgs()<=0) {
			// no parameters => all children
			AbstractNode* node = new AbstractNode(inst);
			for (int n = 0; n < (int)modulectx->numChildren(); ++n) {
				AbstractNode* childnode = modulectx->getChild(n)->evaluate(modulectx);
				if (childnode==NULL) continue; // error
				node->children.push_back(childnode);
			}
			return node;
		}
		else if (evalctx->numArgs()>0) {
			// one (or more ignored) parameter
			const Value& value = evalctx->getArgValue(0);
			if (value.type() == Value::NUMBER) {
				return getChild(value,modulectx);
			}
			else if (value.type() == Value::VECTOR) {
				AbstractNode* node = new AbstractNode(inst);
				const Value::VectorType& vect = value.toVector();
				foreach (const Value::VectorType::value_type& vectvalue, vect) {
					AbstractNode* childnode = getChild(vectvalue,modulectx);
					if (childnode==NULL) continue; // error
					node->children.push_back(childnode);
				}
				return node;
			}
			else if (value.type() == Value::RANGE) {
				AbstractNode* node = new AbstractNode(inst);
				Value::RangeType range = value.toRange();
                                boost::uint32_t steps = range.nbsteps();
				if (steps >= 10000) {
					PRINTB("WARNING: Bad range parameter for children: too many elements (%lu).", steps);
					return NULL;
				}
                                for (Value::RangeType::iterator it = range.begin();it != range.end();it++) {
					AbstractNode* childnode = getChild(Value(*it),modulectx); // with error cases
					if (childnode==NULL) continue; // error
					node->children.push_back(childnode);
				}
				return node;
			}
			else {
				// Invalid parameter
				// (e.g. first child of difference is invalid)
				PRINTB("WARNING: Bad parameter type (%s) for children, only accept: empty, number, vector, range.", value.toString());
				return NULL;
			}
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
		for (size_t i = 0; i < inst->arguments.size(); i++) {
			if (i > 0) msg << ", ";
			if (!evalctx->getArgName(i).empty()) msg << evalctx->getArgName(i) << " = ";
			msg << evalctx->getArgValue(i);
		}
		PRINTB("%s", msg.str());
	}

	if (type == ASSIGN)
	{
		Context c(evalctx);
		for (size_t i = 0; i < evalctx->numArgs(); i++) {
			if (!evalctx->getArgName(i).empty())
				c.set_variable(evalctx->getArgName(i), evalctx->getArgValue(i));
		}
		std::vector<AbstractNode *> instantiatednodes = inst->instantiateChildren(&c);
		node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
	}

	if (type == FOR || type == INT_FOR)
	{
		for_eval(*node, *inst, 0, evalctx, evalctx);
	}

	if (type == IF)
	{
		const IfElseModuleInstantiation *ifelse = dynamic_cast<const IfElseModuleInstantiation*>(inst);
		if (evalctx->numArgs() > 0 && evalctx->getArgValue(0).toBool()) {
			std::vector<AbstractNode *> instantiatednodes = ifelse->instantiateChildren(evalctx);
			node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
		}
		else {
			std::vector<AbstractNode *> instantiatednodes = ifelse->instantiateElseChildren(evalctx);
			node->children.insert(node->children.end(), instantiatednodes.begin(), instantiatednodes.end());
		}
	}

	return node;
}

void register_builtin_control()
{
	Builtins::init("child", new ControlModule(ControlModule::CHILD));
	Builtins::init("children", new ControlModule(ControlModule::CHILDREN));
	Builtins::init("echo", new ControlModule(ControlModule::ECHO));
	Builtins::init("assign", new ControlModule(ControlModule::ASSIGN));
	Builtins::init("for", new ControlModule(ControlModule::FOR));
	Builtins::init("intersection_for", new ControlModule(ControlModule::INT_FOR));
	Builtins::init("if", new ControlModule(ControlModule::IF));
}

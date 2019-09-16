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

#include "function.h"
#include "evalcontext.h"
#include "expression.h"
#include "printutils.h"

#include <typeinfo>
#include <forward_list>

AbstractFunction::~AbstractFunction()
{
}

UserFunction::UserFunction(const char *name, AssignmentList &definition_arguments, shared_ptr<Expression> expr, const Location &loc)
	: ASTNode(loc), name(name), definition_arguments(definition_arguments), expr(expr)
{
}

UserFunction::~UserFunction()
{
}

ValuePtr UserFunction::evaluate(const Context *ctx, const EvalContext *evalctx) const
{
	if (!expr) return ValuePtr::undefined;
	Context c_next(ctx); // Context for next tail call
	c_next.setVariables(evalctx, definition_arguments);

	// Outer loop: to allow tail calls
	unsigned int counter = 0;
	ValuePtr result;
	while (true) {
		// Local contexts for a call. Nested contexts must be supported, to allow variable reassignment in an inner context.
		// I.e. "let(x=33) let(x=42) x" should evaluate to 42.
		// Cannot use std::vector, as it invalidates raw pointers.
		std::forward_list<Context> c_local_stack;
		c_local_stack.emplace_front(&c_next);
		Context *c_local = &c_local_stack.front();

		// Inner loop: to follow a single execution path
		// Before a 'break', must either assign result, or set tailCall to true.
		shared_ptr<Expression> subExpr = expr;
		bool tailCall = false;
		while (true) {
			if (!subExpr) {
				result = ValuePtr::undefined;
				break;
			}
			else if (typeid(*subExpr) == typeid(TernaryOp)) {
				const shared_ptr<TernaryOp> &ternary = static_pointer_cast<TernaryOp>(subExpr);
				subExpr = ternary->evaluateStep(c_local);
			}
			else if (typeid(*subExpr) == typeid(Assert)) {
				const shared_ptr<Assert> &assertion = static_pointer_cast<Assert>(subExpr);
				subExpr = assertion->evaluateStep(c_local);
			}
			else if (typeid(*subExpr) == typeid(Echo)) {
				const shared_ptr<Echo> &echo = static_pointer_cast<Echo>(subExpr);
				subExpr = echo->evaluateStep(c_local);
			}
			else if (typeid(*subExpr) == typeid(Let)) {
				const shared_ptr<Let> &let = static_pointer_cast<Let>(subExpr);
				// Start a new, nested context
				c_local_stack.emplace_front(c_local);
				c_local = &c_local_stack.front();
				subExpr = let->evaluateStep(c_local);
			}
			else if (typeid(*subExpr) == typeid(FunctionCall)) {
				const shared_ptr<FunctionCall> &call = static_pointer_cast<FunctionCall>(subExpr);
				if (call->isLookup && name == call->get_name()) {
					// Update c_next with new parameters for tail call
					call->prepareTailCallContext(c_local, &c_next, definition_arguments);
					tailCall = true;
				}
				else {
					result = subExpr->evaluate(c_local);
				}
				break;
			}
			else {
				result = subExpr->evaluate(c_local);
				break;
			}
		}
		if (!tailCall) {
			break;
		}

		if (counter++ == 1000000){
			std::string locs = loc.toRelativeString(ctx->documentPath());
			PRINTB("ERROR: Recursion detected calling function '%s' %s", this->name % locs);
			throw RecursionException::create("function", this->name,loc);
		}
	}

	return result;
}

void UserFunction::print(std::ostream &stream, const std::string &indent) const
{
	stream << indent << "function " << name << "(";
	for (size_t i=0; i < definition_arguments.size(); i++) {
		const Assignment &arg = definition_arguments[i];
		if (i > 0) stream << ", ";
		stream << arg.name;
		if (arg.expr) stream << " = " << *arg.expr;
	}
	stream << ") = " << *expr << ";\n";
}

BuiltinFunction::~BuiltinFunction()
{
}

ValuePtr BuiltinFunction::evaluate(const Context *ctx, const EvalContext *evalctx) const
{
	return eval_func(ctx, evalctx);
}

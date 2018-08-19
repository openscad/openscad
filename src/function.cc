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
	Context c(ctx);
	c.setVariables(definition_arguments, evalctx);
	ValuePtr result = expr->evaluate(&c);

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

class FunctionTailRecursion : public UserFunction
{
private:
	bool invert;
	shared_ptr<TernaryOp> op;
	shared_ptr<FunctionCall> call;
	shared_ptr<Expression> endexpr;

public:
	FunctionTailRecursion(const char *name, AssignmentList &definition_arguments,
												shared_ptr<TernaryOp> expr, shared_ptr<FunctionCall> call,
												shared_ptr<Expression> endexpr, bool invert,
												const Location &loc)
		: UserFunction(name, definition_arguments, expr, loc),
			invert(invert), op(expr), call(call), endexpr(endexpr) {
	}

	~FunctionTailRecursion() { }

	ValuePtr evaluate(const Context *ctx, const EvalContext *evalctx) const override {
		if (!expr) return ValuePtr::undefined;
		
		Context c(ctx);
		c.setVariables(definition_arguments, evalctx);
		
		EvalContext ec(&c, call->arguments);
		Context tmp(&c);
		unsigned int counter = 0;
		while (invert ^ this->op->cond->evaluate(&c)) {
			tmp.setVariables(definition_arguments, &ec);
			c.apply_variables(tmp);
			
			if (counter++ == 1000000) throw RecursionException::create("function", this->name);
		}
		
		ValuePtr result = endexpr->evaluate(&c);
		
		return result;
	}
};

UserFunction *UserFunction::create(const char *name, AssignmentList &definition_arguments, shared_ptr<Expression> expr, const Location &loc)
{
	if (shared_ptr<TernaryOp> ternary = dynamic_pointer_cast<TernaryOp>(expr)) {
		shared_ptr<FunctionCall> ifcall = dynamic_pointer_cast<FunctionCall>(ternary->ifexpr);
		shared_ptr<FunctionCall> elsecall = dynamic_pointer_cast<FunctionCall>(ternary->elseexpr);
		if (ifcall && !elsecall) {
			if (name == ifcall->name) {
				return new FunctionTailRecursion(name, definition_arguments, ternary, ifcall, ternary->elseexpr, false, loc);
			}
		} else if (elsecall && !ifcall) {
			if (name == elsecall->name) {
				return new FunctionTailRecursion(name, definition_arguments, ternary, elsecall, ternary->ifexpr, true, loc);
			}
		}
	}
	return new UserFunction(name, definition_arguments, expr, loc);
}

BuiltinFunction::~BuiltinFunction()
{
}

ValuePtr BuiltinFunction::evaluate(const Context *ctx, const EvalContext *evalctx) const
{
	return eval_func(ctx, evalctx);
}

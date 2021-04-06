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

Value UserFunction::evaluate(const std::shared_ptr<Context>& ctx, const std::shared_ptr<EvalContext>& evalctx) const
{
	return evaluate_function(name, expr, definition_arguments, ctx, evalctx, loc);
}

void UserFunction::print(std::ostream &stream, const std::string &indent) const
{
	stream << indent << "function " << name << "(";
	for (size_t i=0; i < definition_arguments.size(); ++i) {
		const auto &arg = definition_arguments[i];
		if (i > 0) stream << ", ";
		stream << arg->getName();
		if (arg->getExpr()) stream << " = " << *arg->getExpr();
	}
	stream << ") = " << *expr << ";\n";
}

BuiltinFunction::~BuiltinFunction()
{
}

Value BuiltinFunction::evaluate(const std::shared_ptr<Context>& ctx, const std::shared_ptr<EvalContext>& evalctx) const
{
	return eval_func(ctx, evalctx);
}

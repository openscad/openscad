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

#include "core/function.h"

#include "core/Arguments.h"
#include "core/Expression.h"

#include <ostream>
#include <memory>
#include <cstddef>
#include <utility>

BuiltinFunction::BuiltinFunction(Value(*f)(const std::shared_ptr<const Context>&, const FunctionCall *), const Feature *feature) :
  evaluate(f),
  feature(feature)
{}

BuiltinFunction::BuiltinFunction(Value(*f)(Arguments, const Location&), const Feature *feature) :
  feature(feature)
{
  evaluate = [f] (const std::shared_ptr<const Context>& context, const FunctionCall *call) {
      return f(Arguments(call->arguments, context), call->location());
    };
}

UserFunction::UserFunction(const char *name, AssignmentList& parameters, std::shared_ptr<Expression> expr, const Location& loc)
  : ASTNode(loc), name(name), parameters(parameters), expr(std::move(expr))
{
}

void UserFunction::print(std::ostream& stream, const std::string& indent) const
{
  stream << indent << "function " << name << "(";
  for (size_t i = 0; i < parameters.size(); ++i) {
    const auto& parameter = parameters[i];
    if (i > 0) stream << ", ";
    stream << parameter->getName();
    if (parameter->getExpr()) stream << " = " << *parameter->getExpr();
  }
  stream << ") = " << *expr << ";\n";
}

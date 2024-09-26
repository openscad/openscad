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

#include "core/Arguments.h"

#include <ostream>
#include <memory>
#include "core/Expression.h"

Arguments::Arguments(const AssignmentList& argument_expressions, const std::shared_ptr<const Context>& context) :
  evaluation_session(context->session())
{
  for (const auto& argument_expression : argument_expressions) {
    emplace_back(
      argument_expression->getName().empty() ? boost::none : boost::optional<std::string>(argument_expression->getName()),
      argument_expression->getExpr()->evaluate(context)
      );
  }
}

Arguments Arguments::clone() const
{
  Arguments output(evaluation_session);
  for (const Argument& argument : *this) {
    output.emplace_back(argument.name, argument.value.clone());
  }
  return output;
}

std::ostream& operator<<(std::ostream& stream, const Argument& argument)
{
  if (argument.name) {
    stream << *argument.name << " = ";
  }
  stream << argument.value.toEchoString();
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const Arguments& arguments)
{
  bool first = true;
  for (const auto& argument : arguments) {
    if (first) {
      first = false;
    } else {
      stream << ", ";
    }
    stream << argument;
  }
  return stream;
}

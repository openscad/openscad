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
 *  This program is distributed in the hope that it will be useful,
 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "core/Assignment.h"
#include "core/customizer/Annotation.h"
#include "core/Expression.h"
#include <ostream>
#include <string>

void Assignment::addAnnotations(AnnotationList *annotations)
{
  for (auto& annotation : *annotations) {
    this->annotations.insert({annotation.getName(), &annotation});
  }
}

bool Assignment::hasAnnotations() const
{
  return !annotations.empty();
}

const Annotation *Assignment::annotation(const std::string& name) const
{
  auto found = annotations.find(name);
  return found == annotations.end() ? nullptr : found->second;
}


void Assignment::print(std::ostream& stream, const std::string& indent) const
{
  if (this->hasAnnotations()) {
    const Annotation *group = this->annotation("Group");
    if (group) group->print(stream, indent);
    const Annotation *description = this->annotation("Description");
    if (description) description->print(stream, indent);
    const Annotation *parameter = this->annotation("Parameter");
    if (parameter) parameter->print(stream, indent);
  }
  stream << indent << this->name << " = " << *this->expr << ";\n";
}

std::ostream& operator<<(std::ostream& stream, const AssignmentList& assignments)
{
  bool first = true;
  for (const auto& assignment : assignments) {
    if (first) {
      first = false;
    } else {
      stream << ", ";
    }
    if (!assignment->getName().empty()) {
      stream << assignment->getName();
    }
    if (!assignment->getName().empty() && assignment->getExpr()) {
      stream << " = ";
    }
    if (assignment->getExpr()) {
      stream << *assignment->getExpr();
    }
  }
  return stream;
}

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

#include "core/ExtrudeNode.h"

#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/Children.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "io/fileutils.h"
#include "core/Builtins.h"
#include "handle_dep.h"

#include <cmath>
#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

namespace {
std::shared_ptr<AbstractNode> builtin_extrude(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<ExtrudeNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(),
                                            {"segments"},
                                            {"convexity"});
  parameters.set_caller("extrude");

  parameters["convexity"].getPositiveInt(node->convexity);

  node->has_segments = parameters.validate_integral("segments", node->segments, 0u);
  if (parameters["align"].type() == Value::Type::BOOL)
  {
    node->align = parameters["align"].toBool();
    node->has_align = true;
  }

  children.instantiate(node);

  return node;
}

} // namespace

std::string ExtrudeNode::toString() const
{
  std::ostringstream stream;

  stream << this->name() << "(";
  int paramNo = 0;
  if (this->convexity > 1) {
    stream << "convexity = " << this->convexity;
    paramNo++;
  }
  if (this->has_segments) {
    if (paramNo>0) stream << ", ";
    stream << "segments = " << this->segments;
    paramNo++;
  }
  if (this->has_align) {
    if (paramNo>0) stream << ", ";
    stream << "align = " << (this->align ? "true" : "false");
    paramNo++;
  }
  stream << ")";
  return stream.str();
}

void register_builtin_extrude()
{
  Builtins::init("extrude", new BuiltinModule(builtin_extrude, &Feature::ExperimentalExtrude),
  {
    "extrude(convexity = 1, segments = 0, align=true)",
  });
}

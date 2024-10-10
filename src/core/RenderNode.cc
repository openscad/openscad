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

#include "core/RenderNode.h"
#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/Builtins.h"
#include "core/Children.h"
#include "core/Parameters.h"

#include <utility>
#include <memory>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

static std::shared_ptr<AbstractNode> builtin_render(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<RenderNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"convexity"});
  if (parameters["convexity"].type() == Value::Type::NUMBER) {
    node->convexity = static_cast<int>(parameters["convexity"].toDouble());
  }

  return children.instantiate(node);
}

std::string RenderNode::toString() const
{
  return STR(this->name(), "(convexity = ", convexity, ")");
}

void register_builtin_render()
{
  Builtins::init("render", new BuiltinModule(builtin_render),
  {
    "render(convexity = 1)",
  });
}

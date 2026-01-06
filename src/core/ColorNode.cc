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

#include "core/ColorNode.h"

#include <utility>
#include <memory>
#include <cctype>
#include <cstddef>
#include <string>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign/list_of.hpp>

#include "core/Builtins.h"
#include "core/Children.h"
#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/Parameters.h"
#include "core/ColorUtil.h"
#include "geometry/linalg.h"
#include "utils/printutils.h"

using namespace boost::assign;  // bring 'operator+=()' into scope

static std::shared_ptr<AbstractNode> builtin_color(const ModuleInstantiation *inst, Arguments arguments,
                                                   const Children& children)
{
  auto node = std::make_shared<ColorNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {"c", "alpha"});
  if (parameters["c"].type() == Value::Type::VECTOR) {
    const auto& vec = parameters["c"].toVector();
    Vector4f color;
    for (size_t i = 0; i < 4; ++i) {
      color[i] = i < vec.size() ? (float)vec[i].toDouble() : 1.0f;
      if (color[i] > 1 || color[i] < 0) {
        LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
            "color() expects numbers between 0.0 and 1.0. Value of %1$.1f is out of range", color[i]);
      }
    }
    node->color = color;
  } else if (parameters["c"].type() == Value::Type::STRING) {
    auto colorname = parameters["c"].toString();
    const auto parsed_color = OpenSCAD::parse_color(colorname);
    if (parsed_color) {
      node->color = *parsed_color;
    } else {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unable to parse color \"%1$s\"", colorname);
      LOG(message_group::Info,
          "For a list of valid color names, see the <a href=\"#colorlist\">Color List</a> window.");
    }
  }
  if (parameters["alpha"].type() == Value::Type::NUMBER) {
    node->color.setAlpha(parameters["alpha"].toDouble());
    if (node->color.a() < 0.0f || node->color.a() > 1.0f) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "color() expects alpha between 0.0 and 1.0. Value of %1$.1f is out of range", node->color.a());
    }
  }

  return children.instantiate(node);
}

std::string ColorNode::toString() const
{
  return STR("color([", this->color.r(), ", ", this->color.g(), ", ", this->color.b(), ", ",
             this->color.a(), "])");
}

std::string ColorNode::name() const { return "color"; }

void register_builtin_color()
{
  Builtins::init("color", new BuiltinModule(builtin_color),
                 {
                   "color(c = [r, g, b, a])",
                   "color(c = [r, g, b], alpha = 1.0)",
                   "color(\"#hexvalue\")",
                   "color(\"colorname\", 1.0)",
                 });
}

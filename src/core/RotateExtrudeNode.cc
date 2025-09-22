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

#include "core/RotateExtrudeNode.h"

#include "core/Builtins.h"
#include "core/Children.h"
#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "io/fileutils.h"
#include "handle_dep.h"
#include <ios>
#include <utility>
#include <memory>
#include <cmath>
#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign;  // bring 'operator+=()' into scope

namespace {

std::shared_ptr<AbstractNode> builtin_rotate_extrude(const ModuleInstantiation *inst,
                                                     Arguments arguments, const Children& children)
{
  auto node = std::make_shared<RotateExtrudeNode>(inst);
#ifdef ENABLE_PYTHON
  node->profile_func = NULL;
  node->twist_func = NULL;
#endif

  const Parameters parameters =
    Parameters::parse(std::move(arguments), inst->location(), {"angle", "start", "origin", "scale"},
                      {"convexity", "v", "a", "method"});

  node->fn = parameters["$fn"].toDouble();
  node->fs = parameters["$fs"].toDouble();
  node->fa = parameters["$fa"].toDouble();

  node->convexity = std::max(2, static_cast<int>(parameters["convexity"].toDouble()));
  // If an angle is specified, use it, defaulting to starting at zero.
  // If no angle is specified, use 360 and default to starting at 180.
  // Regardless, if a start angle is specified, use it.
  bool hasAngle = parameters[{"angle", "a"}].getFiniteDouble(node->angle);
  if (hasAngle) {
    node->start = 0;
    if ((node->angle <= -360) || (node->angle > 360)) node->angle = 360;
  } else {
    node->angle = 360;
    node->start = 180;
  }
  bool hasStart = parameters["start"].getFiniteDouble(node->start);
  if (!hasAngle && !hasStart && (int)node->fn % 2 != 0) {
    LOG(message_group::Deprecated,
        "In future releases, rotational extrusion without \"angle\" will start at zero, the +X axis.  "
        "Set start=180 to explicitly start on the -X axis.");
  }

  if (node->convexity <= 0) node->convexity = 2;

  if (node->scale <= 0) node->scale = 1;

  Vector3d v(0, 0, 0);

  if (parameters["v"].isDefined()) {
    if (!parameters["v"].getVec3(v[0], v[1], v[2])) {
      v = Vector3d(0, 0, 0);
      LOG(message_group::Error, "v when specified should be a 3d vector.");
    }
  }
  if (parameters["method"].isUndefined()) {
    node->method = "centered";
  } else {
    node->method = parameters["method"].toString();
    // method can only be one of...
    if (node->method != "centered" && node->method != "linear") {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unknown rotate_extrude method '" + node->method + "'. Using 'centered'.");
      node->method = "centered";
    }
  }
  if (node->angle <= -360) node->angle = 360;
  if (node->angle > 360 && v.norm() == 0) node->angle = 360;

  node->v = v;
  children.instantiate(node);

  return node;
}

}  // namespace

std::string RotateExtrudeNode::toString() const
{
  std::ostringstream stream;

  stream << this->name() << "(";
  if (fabs(origin_x) + fabs(origin_y) > 0)
    stream << "origin = [" << std::dec << this->origin_x << ", " << this->origin_y << "], ";
  if (fabs(offset_x) + fabs(offset_y) > 0)
    stream << "offset = [" << std::dec << this->offset_x << ", " << this->offset_y << "], ";
  if (scale != 1) stream << "scale = " << this->scale << ", ";
  stream << "angle = " << this->angle << ", ";
  if (method != "centered") stream << "method = \"" << this->method << "\", ";
  if (v.norm() > 0)
    stream << "v = [ " << this->v[0] << ", " << this->v[1] << ", " << this->v[2] << "], ";
  stream << "start = " << this->start
         << ", "
            "convexity = "
         << this->convexity << ", ";
#ifdef ENABLE_PYTHON
  if (this->profile_func != NULL) {
    stream << ", profile = " << rand();
  }
  if (this->twist_func != NULL) {
    stream << ", twist_func = " << rand();
  } else
#endif
    if (twist != 0)
    stream << "twist = " << this->twist << ", ";

  stream << "$fn = " << this->fn
         << ", "
            "$fa = "
         << this->fa
         << ", "
            "$fs = "
         << this->fs << ")";

  return stream.str();
}

void register_builtin_rotate_extrude()
{
  Builtins::init("rotate_extrude", new BuiltinModule(builtin_rotate_extrude),
                 {
                   "rotate_extrude(angle = 360, convexity = 2)",
                 });
}

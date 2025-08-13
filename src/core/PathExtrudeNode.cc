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

#include "PathExtrudeNode.h"

#include "module.h"
#include "ModuleInstantiation.h"
#include "Children.h"
#include "Parameters.h"
#include "src/utils/printutils.h"
#include "io/fileutils.h"
#include "Builtins.h"
#include "handle_dep.h"

#include <cmath>
#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign;  // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
// namespace fs = boost::filesystem;

/*
 * Historic path_extrude argument parsing is quirky. To remain bug-compatible,
 * try two different parses depending on conditions.
 */
Parameters parse_parameters_path(Arguments arguments, const Location& location)
{
  {
    Parameters normal_parse =
      Parameters::parse(arguments.clone(), location,
                        {"path", "file", "origin", "scale", "closed", "allow_intersect", "twist",
                         "slices", "segments", "xdir"},
                        {"convexity"});
    if (!(arguments.size() > 0 && !arguments[0].name && arguments[0]->type() == Value::Type::NUMBER)) {
      return normal_parse;
    }
  }

  return Parameters::parse(
    std::move(arguments), location,
    {"origin", "scale", "closed", "allow_intersect", "twist", "slices", "segments"}, {"convexity"});
}

static std::shared_ptr<AbstractNode> builtin_path_extrude(const ModuleInstantiation *inst,
                                                          Arguments arguments, const Children& children)
{
  auto node = std::make_shared<PathExtrudeNode>(inst);

#ifdef ENABLE_PYTHON
  node->profile_func = NULL;
  node->twist_func = NULL;
#endif
  Parameters parameters = parse_parameters_path(std::move(arguments), inst->location());
  parameters.set_caller("path_extrude");

  if (parameters["path"].type() != Value::Type::VECTOR) {
    LOG(message_group::Error, inst->location(), parameters.documentRoot(),
        "Unable to convert path = %1$s to a vector of coordinates",
        parameters["path"].toEchoStringNoThrow());
    return node;
  }
  for (const Value& pointValue : parameters["path"].toVector()) {
    Vector4d point;
    if (pointValue.getVec3(point[0], point[1], point[2]) || !std::isfinite(point[0]) ||
        !std::isfinite(point[1]) || !std::isfinite(point[2])) {
      point[3] = 0.0;
      node->path.push_back(point);
    } else if (pointValue.getVec4(point[0], point[1], point[2], point[3]) || !std::isfinite(point[0]) ||
               !std::isfinite(point[1]) || !std::isfinite(point[2]) || !std::isfinite(point[3])) {
      node->path.push_back(point);
    } else {
      LOG(message_group::Error, inst->location(), parameters.documentRoot(),
          "Unable to convert path[%1$d] = %2$s to a vec3 or vec4 of numbers", node->path.size(),
          pointValue.toEchoStringNoThrow());
      node->path.push_back({0, 0, 0, 0});
    }
  }

  node->fn = parameters["$fn"].toDouble();
  node->fs = parameters["$fs"].toDouble();
  node->fa = parameters["$fa"].toDouble();

  parameters["convexity"].getPositiveInt(node->convexity);

  bool originOk = parameters["origin"].getVec2(node->origin_x, node->origin_y);
  originOk &= std::isfinite(node->origin_x) && std::isfinite(node->origin_y);
  if (parameters["origin"].isDefined() && !originOk) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "path_extrude(..., origin=%1$s) could not be converted",
        parameters["origin"].toEchoStringNoThrow());
  }
  node->scale_x = node->scale_y = 1;
  bool scaleOK = parameters["scale"].getFiniteDouble(node->scale_x);
  scaleOK &= parameters["scale"].getFiniteDouble(node->scale_y);
  scaleOK |= parameters["scale"].getVec2(node->scale_x, node->scale_y, true);
  if ((parameters["scale"].isDefined()) &&
      (!scaleOK || !std::isfinite(node->scale_x) || !std::isfinite(node->scale_y))) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "path_extrude(..., scale=%1$s) could not be converted",
        parameters["scale"].toEchoStringNoThrow());
  }

  if (parameters["closed"].type() == Value::Type::BOOL) node->closed = parameters["closed"].toBool();
  if (parameters["allow_intersect"].type() == Value::Type::BOOL)
    node->allow_intersect = parameters["allow_intersect"].toBool();

  if (node->scale_x < 0) node->scale_x = 0;
  if (node->scale_y < 0) node->scale_y = 0;

  node->xdir_x = 1.0;
  node->xdir_y = 0.0;
  node->xdir_z = 0.0;
  bool xdirOK = parameters["xdir"].getVec3(node->xdir_x, node->xdir_y, node->xdir_z, true);
  if ((parameters["xdir"].isDefined()) &&
      (!xdirOK || !std::isfinite(node->xdir_x) || !std::isfinite(node->xdir_y) ||
       !std::isfinite(node->xdir_z))) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "path_extrude(..., xdir=%1$s) could not be converted", parameters["xdir"].toEchoStringNoThrow());
  }
  node->has_slices = parameters.validate_integral("slices", node->slices, 1u);
  node->has_segments = parameters.validate_integral("segments", node->segments, 0u);

  node->twist = 0.0;
  parameters["twist"].getFiniteDouble(node->twist);
  if (node->twist != 0.0) {
    node->has_twist = true;
  }

  children.instantiate(node);

  return node;
}

std::string PathExtrudeNode::toString() const
{
  std::ostringstream stream;

  stream << this->name() << "(";
  if (this->has_twist) {
    stream << ", twist = " << this->twist;
  }
  if (this->has_slices) {
    stream << ", slices = " << this->slices;
  }
  if (this->has_segments) {
    stream << ", segments = " << this->segments;
  }

  if (this->scale_x != this->scale_y) {
    stream << ", scale = [" << this->scale_x << ", " << this->scale_y << "]";
  } else if (this->scale_x != 1.0) {
    stream << ", scale = " << this->scale_x;
  }

  if (!(this->has_slices && this->has_segments)) {
    stream << ", $fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs;
  }
  if (this->convexity > 1) {
    stream << ", convexity = " << this->convexity;
  }
  if (this->path.size() > 0) {
    stream << ", path = ";
    for (size_t i = 0; i < this->path.size(); i++) {
      stream << this->path[i][0] << " " << this->path[i][1] << " " << this->path[i][2] << " "
             << this->path[i][3] << ", ";
    }
  }
  stream << ", xdir = " << this->xdir_x << " " << this->xdir_y << " " << this->xdir_z;
  stream << ", closed = " << this->closed;
  stream << ", allow_intersect = " << this->allow_intersect;

#ifdef ENABLE_PYTHON
  if (this->profile_func != NULL) {
    stream << ", profile = " << rand();
  }
  if (this->twist_func != NULL) {
    stream << ", twist_func = " << rand();
  }
#endif
  stream << ")";
  return stream.str();
}

void register_builtin_path_extrude()
{
  Builtins::init("path_extrude", new BuiltinModule(builtin_path_extrude));
}

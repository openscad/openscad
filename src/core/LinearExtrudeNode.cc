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

#include "core/LinearExtrudeNode.h"

#include "core/Children.h"
#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "io/fileutils.h"
#include "core/Builtins.h"
#include "handle_dep.h"

#include <utility>
#include <memory>
#include <cmath>
#include <sstream>

#include <filesystem>

namespace {
std::shared_ptr<AbstractNode> builtin_linear_extrude(const ModuleInstantiation *inst,
                                                     Arguments arguments, const Children& children)
{
  Parameters parameters = Parameters::parse(
    std::move(arguments), inst->location(),
    {"height", "v", "scale", "center", "twist", "slices", "segments"}, {"convexity", "h"});
  parameters.set_caller("linear_extrude");

  auto node = std::make_shared<LinearExtrudeNode>(inst, CurveDiscretizer(parameters, inst->location()));

  double height = 100.0;

  if (parameters["v"].isDefined()) {
    if (!parameters["v"].getVec3(node->height[0], node->height[1], node->height[2])) {
      LOG(message_group::Error, "v when specified should be a 3d vector.");
    }
    height = 1.0;
  }
  const Value& heightValue = parameters[{"height", "h"}];
  if (heightValue.isDefined()) {
    if (!heightValue.getFiniteDouble(height)) {
      LOG(message_group::Error, "height when specified should be a number.");
      height = 100.0;
    }
    node->height.normalize();
  }
  node->height *= height;

  parameters["convexity"].getPositiveInt(node->convexity);

  node->scale_x = node->scale_y = 1;
  bool scaleOK = parameters["scale"].getFiniteDouble(node->scale_x);
  scaleOK &= parameters["scale"].getFiniteDouble(node->scale_y);
  scaleOK |= parameters["scale"].getVec2(node->scale_x, node->scale_y, true);
  if ((parameters["scale"].isDefined()) &&
      (!scaleOK || !std::isfinite(node->scale_x) || !std::isfinite(node->scale_y))) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "linear_extrude(..., scale=%1$s) could not be converted",
        parameters["scale"].toEchoStringNoThrow());
  }

  if (parameters["center"].type() == Value::Type::BOOL) node->center = parameters["center"].toBool();

  if (node->height[2] <= 0) node->height[2] = 0;

  if (node->scale_x < 0) node->scale_x = 0;
  if (node->scale_y < 0) node->scale_y = 0;

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

}  // namespace

std::string LinearExtrudeNode::toString() const
{
  std::ostringstream stream;

  stream << this->name() << "(";
  double height = this->height.norm();
  stream << "height = " << height;
  if (height > 0) {
    Vector3d v = this->height / height;
    if (v[2] < 1) {
      stream << ", v = [ " << v[0] << ", " << v[1] << ", " << v[2] << "]";
    }
  }
  if (this->center) {
    stream << ", center = true";
  }
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
    stream << ", " << this->discretizer;
  }
  if (this->convexity > 1) {
    stream << ", convexity = " << this->convexity;
  }
  stream << ")";
  return stream.str();
}

void register_builtin_linear_extrude()
{
  Builtins::init("linear_extrude", new BuiltinModule(builtin_linear_extrude),
                 {
                   "linear_extrude(height = 100, center = false, convexity = 1, twist = 0, scale = 1.0, "
                   "[slices, segments, v])",
                 });
}

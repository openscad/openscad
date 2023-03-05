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

#include "RotateExtrudeNode.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "Children.h"
#include "Parameters.h"
#include "printutils.h"
#include "io/fileutils.h"
#include "Builtins.h"
#include "handle_dep.h"
#include <cmath>
#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

static std::shared_ptr<AbstractNode> builtin_rotate_extrude(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<RotateExtrudeNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(),
                                            {"file", "layer", "origin", "scale"},
                                            {"convexity", "angle", "translate"}
                                            );

  node->fn = parameters["$fn"].toDouble();
  node->fs = parameters["$fs"].toDouble();
  node->fa = parameters["$fa"].toDouble();

  if (!parameters["file"].isUndefined()) {
    LOG(message_group::Deprecated, "Support for reading files in rotate_extrude will be removed in future releases. Use a child import() instead.");
    auto filename = lookup_file(parameters["file"].toString(), inst->location().filePath().parent_path().string(), parameters.documentRoot());
    node->filename = filename;
    handle_dep(filename);
  }

  node->layername = parameters["layer"].isUndefined() ? "" : parameters["layer"].toString();
  node->convexity = static_cast<int>(parameters["convexity"].toDouble());
  bool originOk = parameters["origin"].getVec2(node->origin_x, node->origin_y);
  originOk &= std::isfinite(node->origin_x) && std::isfinite(node->origin_y);
  if (parameters["origin"].isDefined() && !originOk) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "rotate_extrude(..., origin=%1$s) could not be converted", parameters["origin"].toEchoStringNoThrow());
  }
  node->scale = parameters["scale"].toDouble();
  node->angle = 360;
  parameters["angle"].getFiniteDouble(node->angle);
  node->scale_x = node->scale_y = 1;
  bool scaleOK = parameters["scale"].getFiniteDouble(node->scale_x);
  scaleOK &= parameters["scale"].getFiniteDouble(node->scale_y);
  scaleOK |= parameters["scale"].getVec2(node->scale_x, node->scale_y, true);
  if (!node->filename.empty() && (node->scale_x != 1 || node->scale_y != 1)) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "rotate_extrude(..., scale=%1$s) interpreted in deprecated mode", parameters["scale"].toEchoStringNoThrow());
    node->scale_x = node->scale_y = 1;
  } else if ((parameters["scale"].isDefined()) && (!scaleOK || !std::isfinite(node->scale_x) || !std::isfinite(node->scale_y))) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "rotate_extrude(..., scale=%1$s) could not be converted", parameters["scale"].toEchoStringNoThrow());
  }
  node->trans_x = node->trans_y = 0;
  bool transOK = parameters["translate"].getFiniteDouble(node->trans_x);
  transOK &= parameters["translate"].getFiniteDouble(node->trans_y);
  transOK |= parameters["translate"].getVec2(node->trans_x, node->trans_y, true);
  if ((parameters["translate"].isDefined()) && (!transOK || !std::isfinite(node->trans_x) || !std::isfinite(node->trans_y))) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "rotate_extrude(..., translate=%1$s) could not be converted", parameters["translate"].toEchoStringNoThrow());
  }

  if (node->convexity <= 0) node->convexity = 2;

  if (node->scale <= 0) node->scale = 1;

  if (node->scale_x == 1 && node->scale_y == 1 && node->trans_x == 0 && node->trans_y == 0 && ((node->angle <= -360) || (node->angle > 360))) node->angle = 360;

  if (node->filename.empty()) {
    children.instantiate(node);
  } else if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "module %1$s() does not support child modules when importing a file", inst->name());
  }


  return node;
}

std::string RotateExtrudeNode::toString() const
{
  std::ostringstream stream;

  stream << this->name() << "(";
  if (!this->filename.empty()) { // Ignore deprecated parameters if empty
    fs::path path((std::string)this->filename);
    stream <<
      "file = " << this->filename << ", "
      "layer = " << QuotedString(this->layername) << ", "
      "origin = [" << std::dec << this->origin_x << ", " << this->origin_y << "], "
      "scale = " << this->scale << ", "
           << "timestamp = " << (fs::exists(path) ? fs::last_write_time(path) : 0) << ", "
    ;
  }
  stream <<
    "angle = " << this->angle << ", ";
  if (this->scale_x != this->scale_y) {
    stream << "scale = [" << this->scale_x << ", " << this->scale_y << "], ";
  } else if (this->scale_x != 1.0) {
    stream << "scale = " << this->scale_x << ", ";
  }
  if (this->trans_x != this->trans_y) {
    stream << "translate = [" << this->trans_x << ", " << this->trans_y << "], ";
  } else if (this->trans_x != 0.0) {
    stream << "translate = " << this->trans_x << ", ";
  }
  stream <<
    "convexity = " << this->convexity << ", "
    "$fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs << ")";

  return stream.str();
}

void register_builtin_dxf_rotate_extrude()
{
  Builtins::init("dxf_rotate_extrude", new BuiltinModule(builtin_rotate_extrude));

  Builtins::init("rotate_extrude", new BuiltinModule(builtin_rotate_extrude),
  {
    "rotate_extrude(angle = 360, scale = 1, translate = 0, convexity = 2)",
  });
}

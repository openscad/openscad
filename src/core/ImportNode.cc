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

#include "core/ImportNode.h"

#include "geometry/Geometry.h"
#include "io/import.h"

#include "geometry/PolySet.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALNefGeometry.h"
#endif
#include "geometry/Polygon2d.h"
#include "core/Builtins.h"
#include "core/Children.h"
#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/Parameters.h"
#include "io/DxfData.h"
#include "io/fileutils.h"
#include "utils/printutils.h"
#include "Feature.h"
#include "handle_dep.h"

#include <cmath>
#include <ios>
#include <utility>
#include <memory>
#include <sys/types.h>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <filesystem>
namespace fs = std::filesystem;
#include <boost/assign/std/vector.hpp>
using namespace boost::assign;  // bring 'operator+=()' into scope

static std::shared_ptr<AbstractNode> do_import(const ModuleInstantiation *inst, Arguments arguments,
                                               ImportType type)
{
  Parameters parameters = Parameters::parse(
    std::move(arguments), inst->location(), {"file", "layer", "convexity", "origin", "scale"},
    {"width", "height", "filename", "layername", "center", "dpi", "id"});

  const auto& v = parameters["file"];
  std::string filename;
  if (v.isDefined()) {
    filename =
      lookup_file(v.isUndefined() ? "" : v.toString(),
                  inst->location().filePath().parent_path().string(), parameters.documentRoot());
  } else {
    const auto& filename_val = parameters["filename"];
    if (!filename_val.isUndefined()) {
      LOG(message_group::Deprecated, "filename= is deprecated. Please use file=");
    }
    filename =
      lookup_file(filename_val.isUndefined() ? "" : filename_val.toString(),
                  inst->location().filePath().parent_path().string(), parameters.documentRoot());
  }
  if (!filename.empty()) handle_dep(filename);
  ImportType actualtype = type;
  if (actualtype == ImportType::UNKNOWN) {
    std::string extraw = fs::path(filename).extension().generic_string();
    std::string ext = boost::algorithm::to_lower_copy(extraw);
    if (ext == ".stl") actualtype = ImportType::STL;
    else if (ext == ".off") actualtype = ImportType::OFF;
    else if (ext == ".dxf") actualtype = ImportType::DXF;
    else if (ext == ".nef3") actualtype = ImportType::NEF3;
    else if (ext == ".3mf") actualtype = ImportType::_3MF;
    else if (ext == ".amf") actualtype = ImportType::AMF;
    else if (ext == ".svg") actualtype = ImportType::SVG;
    else if (ext == ".obj") actualtype = ImportType::OBJ;
  }

  auto node =
    std::make_shared<ImportNode>(inst, actualtype, CurveDiscretizer(parameters, inst->location()));

  node->filename = filename;
  const auto& layerval = parameters["layer"];
  if (layerval.isDefined()) {
    node->layer = layerval.toString();
  } else {
    const auto& layername = parameters["layername"];
    if (layername.isDefined()) {
      LOG(message_group::Deprecated, "layername= is deprecated. Please use layer=");
      node->layer = layername.toString();
    }
  }
  const auto& idval = parameters["id"];
  if (idval.isDefined()) {
    node->id = idval.toString();
  }
  node->convexity = (int)parameters["convexity"].toDouble();

  if (node->convexity <= 0) node->convexity = 1;

  const auto& origin = parameters["origin"];
  node->origin_x = node->origin_y = 0;
  bool originOk = origin.getVec2(node->origin_x, node->origin_y);
  originOk &= std::isfinite(node->origin_x) && std::isfinite(node->origin_y);
  if (origin.isDefined() && !originOk) {
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
        "Unable to convert import(..., origin=%1$s) parameter to vec2", origin.toEchoStringNoThrow());
  }

  const auto& center = parameters["center"];
  node->center = center.type() == Value::Type::BOOL ? center.toBool() : false;

  node->scale = parameters["scale"].toDouble();
  if (node->scale <= 0) node->scale = 1;

  node->dpi = ImportNode::SVG_DEFAULT_DPI;
  const auto& dpi = parameters["dpi"];
  if (dpi.type() == Value::Type::NUMBER) {
    double val = dpi.toDouble();
    if (val < 0.001) {
      std::string filePath =
        fs_uncomplete(inst->location().filePath(), parameters.documentRoot()).generic_string();
      LOG(message_group::Warning,
          "Invalid dpi value giving, using default of %1$f dpi. Value must be positive and >= 0.001, "
          "file %2$s, import() at line %3$d",
          origin.toEchoStringNoThrow(), filePath, filePath, inst->location().firstLine());
    } else {
      node->dpi = val;
    }
  }

  node->width = parameters.get("width", -1);
  node->height = parameters.get("height", -1);

  return node;
}

static std::shared_ptr<AbstractNode> builtin_import(const ModuleInstantiation *inst, Arguments arguments)
{
  return do_import(inst, std::move(arguments), ImportType::UNKNOWN);
}

template <typename T>
static std::unique_ptr<T> optionally_center(std::unique_ptr<T> g, bool center)
{
  if (center) {
    auto bbox = g->getBoundingBox();
    auto center = bbox.center();

    if constexpr (std::is_same_v<T, Polygon2d>) {
      auto mat = Transform2d::Identity();
      auto translate = mat.translation();

      translate.x() = -center.x();
      translate.y() = -center.y();

      g->transform(mat);
    } else {
      auto mat = Transform3d::Identity();
      auto translate = mat.translation();

      translate.x() = -center.x();
      translate.y() = -center.y();
      translate.z() = -center.z();

      g->transform(mat);
    }
  }

  return g;
}

/*!
   Will return an empty geometry if the import failed, but not nullptr
 */
std::unique_ptr<const Geometry> ImportNode::createGeometry() const
{
  std::unique_ptr<Geometry> g;
  auto loc = this->modinst->location();

  switch (this->type) {
  case ImportType::STL: {
    g = optionally_center(import_stl(this->filename, loc), this->center);
    break;
  }
  case ImportType::AMF: {
    g = optionally_center(import_amf(this->filename, loc), this->center);
    break;
  }
  case ImportType::_3MF: {
    g = optionally_center(import_3mf(this->filename, loc), this->center);
    break;
  }
  case ImportType::OFF: {
    g = optionally_center(import_off(this->filename, loc), this->center);
    break;
  }
  case ImportType::OBJ: {
    g = optionally_center(import_obj(this->filename, loc), this->center);
    break;
  }
  case ImportType::SVG: {
    g =
      import_svg(this->discretizer, this->filename, this->id, this->layer, this->dpi, this->center, loc);
    break;
  }
  case ImportType::DXF: {
    DxfData dd(this->discretizer, this->filename, this->layer.value_or(""), this->origin_x,
               this->origin_y, this->scale);
    g = optionally_center(dd.toPolygon2d(), this->center);
    break;
  }
#ifdef ENABLE_CGAL
  case ImportType::NEF3: {
    g = import_nef3(this->filename, loc);
    break;
  }
#endif
  default:
    LOG(message_group::Error,
        "Unsupported file format while trying to import file '%1$s', import() at line %2$d",
        this->filename, loc.firstLine());
    g = PolySet::createEmpty();
  }

  g->setConvexity(this->convexity);
  return g;
}

std::string ImportNode::toString() const
{
  std::ostringstream stream;
  fs::path path((std::string)this->filename);

  stream << this->name();
  stream << "(file = " << this->filename;
  if (this->id) {
    stream << ", id = " << QuotedString(this->id.get());
  }
  if (this->layer) {
    stream << ", layer = " << QuotedString(this->layer.get());
  }
  stream << ", origin = [" << std::dec << this->origin_x << ", " << this->origin_y << "]";
  if (this->type == ImportType::SVG) {
    stream << ", dpi = " << this->dpi;
  }
  stream << ", scale = " << this->scale << ", center = " << (this->center ? "true" : "false")
         << ", convexity = " << this->convexity << ", " << this->discretizer
         << ", timestamp = " << fs_timestamp(path) << ")";

  return stream.str();
}

std::string ImportNode::name() const { return "import"; }

void register_builtin_import()
{
  Builtins::init("import", new BuiltinModule(builtin_import),
                 {
                   "import(string, [number, [number]])",
                 });
}

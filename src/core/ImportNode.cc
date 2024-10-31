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

#include "io/import.h"

#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "geometry/PolySet.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#endif
#include "geometry/Polygon2d.h"
#include "core/Builtins.h"
#include "core/Children.h"
#include "io/DxfData.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "io/fileutils.h"
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
using namespace boost::assign; // bring 'operator+=()' into scope


static std::shared_ptr<AbstractNode> do_import(const ModuleInstantiation *inst, Arguments arguments, const Children& children, ImportType type)
{
  if (!children.empty()) {
    LOG(message_group::Warning, inst->location(), arguments.documentRoot(),
        "module %1$s() does not support child modules", inst->name());
  }

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(),
                                            {"file", "layer", "convexity", "origin", "scale"},
                                            {"width", "height", "filename", "layername", "center", "dpi", "id"}
                                            );

  const auto& v = parameters["file"];
  std::string filename;
  if (v.isDefined()) {
    filename = lookup_file(v.isUndefined() ? "" : v.toString(), inst->location().filePath().parent_path().string(), parameters.documentRoot());
  } else {
    const auto& filename_val = parameters["filename"];
    if (!filename_val.isUndefined()) {
      LOG(message_group::Deprecated, "filename= is deprecated. Please use file=");
    }
    filename = lookup_file(filename_val.isUndefined() ? "" : filename_val.toString(), inst->location().filePath().parent_path().string(), parameters.documentRoot());
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

  auto node = std::make_shared<ImportNode>(inst, actualtype);

  node->fn = parameters["$fn"].toDouble();
  node->fs = parameters["$fs"].toDouble();
  node->fa = parameters["$fa"].toDouble();

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
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Unable to convert import(..., origin=%1$s) parameter to vec2", origin.toEchoStringNoThrow());
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
      std::string filePath = fs_uncomplete(inst->location().filePath(), parameters.documentRoot()).generic_string();
      LOG(message_group::Warning,
          "Invalid dpi value giving, using default of %1$f dpi. Value must be positive and >= 0.001, file %2$s, import() at line %3$d",
          origin.toEchoStringNoThrow(), filePath, filePath, inst->location().firstLine()
          );
    } else {
      node->dpi = val;
    }
  }

  node->width = parameters.get("width", -1);
  node->height = parameters.get("height", -1);

  return node;
}

static std::shared_ptr<AbstractNode> builtin_import(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{ return do_import(inst, std::move(arguments), children, ImportType::UNKNOWN); }

static std::shared_ptr<AbstractNode> builtin_import_stl(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{ return do_import(inst, std::move(arguments), children, ImportType::STL); }

static std::shared_ptr<AbstractNode> builtin_import_off(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{ return do_import(inst, std::move(arguments), children, ImportType::OFF); }

static std::shared_ptr<AbstractNode> builtin_import_dxf(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{ return do_import(inst, std::move(arguments), children, ImportType::DXF); }



/*!
   Will return an empty geometry if the import failed, but not nullptr
 */
std::unique_ptr<const Geometry> ImportNode::createGeometry() const
{
  std::unique_ptr<Geometry> g;
  auto loc = this->modinst->location();

  switch (this->type) {
  case ImportType::STL: {
    g = import_stl(this->filename, loc);
    break;
  }
  case ImportType::AMF: {
    g = import_amf(this->filename, loc);
    break;
  }
  case ImportType::_3MF: {
    g = import_3mf(this->filename, loc);
    break;
  }
  case ImportType::OFF: {
    g = import_off(this->filename, loc);
    break;
  }
  case ImportType::OBJ: {
    g = import_obj(this->filename, loc);
    break;
  }
  case ImportType::SVG: {
    g = import_svg(this->fn, this->fs, this->fa, this->filename, this->id, this->layer, this->dpi, this->center, loc);
    break;
  }
  case ImportType::DXF: {
    DxfData dd(this->fn, this->fs, this->fa, this->filename, this->layer.value_or(""), this->origin_x, this->origin_y, this->scale);
    g = dd.toPolygon2d();
    break;
  }
#ifdef ENABLE_CGAL
  case ImportType::NEF3: {
    g = import_nef3(this->filename, loc);
    break;
  }
#endif
  default:
    LOG(message_group::Error, "Unsupported file format while trying to import file '%1$s', import() at line %2$d", this->filename, loc.firstLine());
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
    stream << ", center = " << (this->center ? "true" : "false")
           << ", dpi = " << this->dpi;
  }
  stream << ", scale = " << this->scale
         << ", convexity = " << this->convexity
         << ", $fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs
         << ", timestamp = " << fs_timestamp(path)
         << ")";

  return stream.str();
}

std::string ImportNode::name() const
{
  return "import";
}

void register_builtin_import()
{
  Builtins::init("import_stl", new BuiltinModule(builtin_import_stl));
  Builtins::init("import_off", new BuiltinModule(builtin_import_off));
  Builtins::init("import_dxf", new BuiltinModule(builtin_import_dxf));

  Builtins::init("import", new BuiltinModule(builtin_import),
  {
    "import(string, [number, [number]])",
  });
}

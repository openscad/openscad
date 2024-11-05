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

#include "io/dxfdim.h"
#include "core/Value.h"
#include "core/function.h"
#include "io/DxfData.h"
#include "core/Builtins.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "io/fileutils.h"
#include "handle_dep.h"
#include "utils/degree_trig.h"

#include <unordered_map>
#include <utility>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <filesystem>
std::unordered_map<std::string, double> dxf_dim_cache;
std::unordered_map<std::string, std::vector<double>> dxf_cross_cache;
namespace fs = std::filesystem;

Value builtin_dxf_dim(Arguments arguments, const Location& loc)
{
  Parameters parameters = Parameters::parse(std::move(arguments), loc, {}, {"file", "layer", "origin", "scale", "name"});

  std::string rawFilename;
  std::string filename;
  if (parameters.contains("file")) {
    rawFilename = parameters["file"].toString();
    filename = lookup_file(rawFilename, loc.filePath().parent_path().string(), parameters.documentRoot());
  }
  double xorigin = 0;
  double yorigin = 0;
  if (parameters.contains("origin")) {
    bool originOk = parameters["origin"].getVec2(xorigin, yorigin);
    originOk &= std::isfinite(xorigin) && std::isfinite(yorigin);
    if (!originOk) {
      LOG(message_group::Warning, loc, parameters.documentRoot(), "dxf_dim(..., origin=%1$s) could not be converted", parameters["origin"].toEchoString());
    }
  }
  std::string layername = parameters.get("layer", "");
  double scale = parameters.get("scale", 1);
  std::string name = parameters.get("name", "");

  fs::path filepath(filename);
  uintmax_t filesize = -1;
  int64_t lastwritetime = -1;
  if (fs::exists(filepath)) {
    if (fs::is_regular_file(filepath)) {
      filesize = fs::file_size(filepath);
      lastwritetime = fs_timestamp(filepath);
    }
  } else {
    LOG(message_group::Warning, loc, parameters.documentRoot(), "Can't open DXF file '%1$s'!", rawFilename);
    return Value::undefined.clone();
  }
  std::string key = STR(filename, "|", layername, "|", name, "|", xorigin,
                        "|", yorigin, "|", scale, "|", lastwritetime,
                        "|", filesize);
  auto result = dxf_dim_cache.find(key);
  if (result != dxf_dim_cache.end()) return {result->second};
  handle_dep(filepath.string());
  DxfData dxf(36, 0, 0, filename, layername, xorigin, yorigin, scale);

  for (auto& dim : dxf.dims) {
    if (!name.empty() && dim.name != name) continue;

    DxfData::Dim *d = &dim;
    int type = d->type & 7;

    if (type == 0) {
      // Rotated, horizontal or vertical
      double x = d->coords[4][0] - d->coords[3][0];
      double y = d->coords[4][1] - d->coords[3][1];
      double angle = d->angle;
      double distance_projected_on_line = std::fabs(x * cos_degrees(angle) + y * sin_degrees(angle));
      dxf_dim_cache.emplace(key, distance_projected_on_line);
      return {distance_projected_on_line};
    } else if (type == 1) {
      // Aligned
      double x = d->coords[4][0] - d->coords[3][0];
      double y = d->coords[4][1] - d->coords[3][1];
      double value = sqrt(x * x + y * y);
      dxf_dim_cache.emplace(key, value);
      return {value};
    } else if (type == 2) {
      // Angular
      double a1 = atan2_degrees(d->coords[0][0] - d->coords[5][0], d->coords[0][1] - d->coords[5][1]);
      double a2 = atan2_degrees(d->coords[4][0] - d->coords[3][0], d->coords[4][1] - d->coords[3][1]);
      double value = std::fabs(a1 - a2);
      dxf_dim_cache.emplace(key, value);
      return {value};
    } else if (type == 3 || type == 4) {
      // Diameter or Radius
      double x = d->coords[5][0] - d->coords[0][0];
      double y = d->coords[5][1] - d->coords[0][1];
      double value = sqrt(x * x + y * y);
      dxf_dim_cache.emplace(key, value);
      return {value};
    } else if (type == 5) {
      // Angular 3 Point
    } else if (type == 6) {
      // Ordinate
      double value = (d->type & 64) ? d->coords[3][0] : d->coords[3][1];
      dxf_dim_cache.emplace(key, value);
      return {value};
    }

    LOG(message_group::Warning, loc, parameters.documentRoot(), "Dimension '%1$s' in '%2$s', layer '%3$s' has unsupported type!", name, rawFilename, layername);
    return Value::undefined.clone();
  }

  LOG(message_group::Warning, loc, parameters.documentRoot(), "Can't find dimension '%1$s' in '%2$s', layer '%3$s'!", name, rawFilename, layername);

  return Value::undefined.clone();
}

Value builtin_dxf_cross(Arguments arguments, const Location& loc)
{
  auto *session = arguments.session();
  Parameters parameters = Parameters::parse(std::move(arguments), loc, {}, {"file", "layer", "origin", "scale", "name"});

  std::string rawFilename;
  std::string filename;
  if (parameters.contains("file")) {
    rawFilename = parameters["file"].toString();
    filename = lookup_file(rawFilename, loc.filePath().parent_path().string(), parameters.documentRoot());
  }
  double xorigin = 0;
  double yorigin = 0;
  if (parameters.contains("origin")) {
    bool originOk = parameters["origin"].getVec2(xorigin, yorigin);
    originOk &= std::isfinite(xorigin) && std::isfinite(yorigin);
    if (!originOk) {
      LOG(message_group::Warning, loc, parameters.documentRoot(), "dxf_cross(..., origin=%1$s) could not be converted", parameters["origin"].toEchoString());
    }
  }
  std::string layername = parameters.get("layer", "");
  double scale = parameters.get("scale", 1);

  fs::path filepath(filename);
  uintmax_t filesize = -1;
  int64_t lastwritetime = -1;
  if (fs::exists(filepath)) {
    if (fs::is_regular_file(filepath)) {
      filesize = fs::file_size(filepath);
      lastwritetime = fs_timestamp(filepath);
    }
  } else {
    LOG(message_group::Warning, loc, parameters.documentRoot(), "Can't open DXF file '%1$s'!", rawFilename);
    return Value::undefined.clone();
  }

  std::string key = STR(filename, "|", layername, "|", xorigin, "|", yorigin,
                        "|", scale, "|", lastwritetime,
                        "|", filesize);

  auto result = dxf_cross_cache.find(key);
  if (result != dxf_cross_cache.end()) {
    VectorType ret(session);
    ret.reserve(result->second.size());
    for (auto v : result->second) {
      ret.emplace_back(v);
    }
    return {std::move(ret)};
  }
  handle_dep(filepath.string());
  DxfData dxf(36, 0, 0, filename, layername, xorigin, yorigin, scale);

  double coords[4][2];

  for (size_t i = 0, j = 0; i < dxf.paths.size(); ++i) {
    if (dxf.paths[i].indices.size() != 2) continue;
    coords[j][0] = dxf.points[dxf.paths[i].indices[0]][0];
    coords[j++][1] = dxf.points[dxf.paths[i].indices[0]][1];
    coords[j][0] = dxf.points[dxf.paths[i].indices[1]][0];
    coords[j++][1] = dxf.points[dxf.paths[i].indices[1]][1];

    if (j == 4) {
      double x1 = coords[0][0], y1 = coords[0][1];
      double x2 = coords[1][0], y2 = coords[1][1];
      double x3 = coords[2][0], y3 = coords[2][1];
      double x4 = coords[3][0], y4 = coords[3][1];
      double dem = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1);
      if (dem == 0) break;
      double ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / dem;
      // double ub = ((x2 - x1)*(y1 - y3) - (y2 - y1)*(x1 - x3)) / dem;
      double x = x1 + ua * (x2 - x1);
      double y = y1 + ua * (y2 - y1);

      std::vector<double> value = {x, y};
      dxf_cross_cache.emplace(key, value);
      VectorType ret(session);
      ret.reserve(2);
      ret.emplace_back(x);
      ret.emplace_back(y);
      return {std::move(ret)};
    }
  }

  LOG(message_group::Warning, loc, parameters.documentRoot(), "Can't find cross in '%1$s', layer '%2$s'!", rawFilename, layername);
  return Value::undefined.clone();
}

void initialize_builtin_dxf_dim()
{
  Builtins::init("dxf_dim", new BuiltinFunction(&builtin_dxf_dim),
  {
    "dxf_dim()",
  });

  Builtins::init("dxf_cross", new BuiltinFunction(&builtin_dxf_cross),
  {
    "dxf_cross()",
  });
}

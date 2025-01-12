/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2021 Clifford Wolf <clifford@clifford.at> and
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

#include <iostream>
#include <string>
#include "json/json.hpp"
#include <boost/property_tree/json_parser.hpp>

#include "io/export.h"
#include "core/customizer/ParameterObject.h"

using json = nlohmann::json;

bool export_param(SourceFile *sourceFile, const fs::path& path, std::ostream& output)
{
  const ParameterObjects parameters = ParameterObjects::fromSourceFile(sourceFile);

  json params;
  for (auto& param : parameters) {
    const std::string description = param->description();
    const std::string group = param->group();

    json o;
    o["name"] = param->name();
    if (!description.empty()) {
      o["caption"] = description;
    }
    if (!group.empty()) {
      o["group"] = group;
    }
    o.merge_patch(param->jsonValue());
    params.push_back(o);
  }

  json paramFile;
  paramFile["title"] = path.has_stem() ? path.stem().generic_string() : "Unnamed";
  if (params.size() > 0) {
    paramFile["parameters"] = params;
  }
  output << paramFile;

  return true;
}

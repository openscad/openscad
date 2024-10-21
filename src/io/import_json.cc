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

#include <exception>
#include <utility>
#include <fstream>
#include <string>
#include "json/json.hpp"

#include "core/Value.h"
#include "utils/printutils.h"
#include "core/EvaluationSession.h"

using json = nlohmann::json;

namespace {

ObjectType to_obj(const json& j, EvaluationSession *session);

Value to_value(const json& j, EvaluationSession *session)
{
  if (j.is_string()) {
    return Value{j.get<std::string>()};
  } else if (j.is_number()) {
    return Value{j.get<double>()};
  } else if (j.is_boolean()) {
    return Value{j.get<bool>()};
  } else if (j.is_object()) {
    return Value{to_obj(j, session)};
  } else if (j.is_array()) {
    Value::VectorType vec{session};
    for (const auto& elem : j) {
      vec.emplace_back(to_value(elem, session));
    }
    return std::move(vec);
  }
  return Value::undefined.clone();
}

ObjectType to_obj(const json& j, EvaluationSession *session)
{
  ObjectType obj{session};
  for (const auto& item : j.items()) {
    obj.set(item.key(), to_value(item.value(), session));
  }
  return obj;
}

} // namespace

Value import_json(const std::string& filename, EvaluationSession *session, const Location& loc)
{
  std::ifstream i(filename);

  try {
    if (i) {
      json j;
      i >> j;
      return Value{to_value(j, session)};
    } else {
      LOG(message_group::Warning, loc, "", "Could not read file '%1$s'", filename);
    }
  } catch (const std::exception& e) {
    LOG(message_group::Warning, loc, "", "Failed to parse file '%1$s': %s", filename, e.what());
  }

  return Value::undefined.clone();
}

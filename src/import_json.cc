/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2019 Clifford Wolf <clifford@clifford.at> and
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

#include <fstream>
#include <json.hpp>

#include "value.h"
#include "printutils.h"

using json = nlohmann::json;

namespace {

ObjectType to_obj(const json& j);

ValuePtr to_value(const json& j)
{
	if (j.is_string()) {
		return ValuePtr{j.get<std::string>()};
	} else if (j.is_number()) {
		return ValuePtr{j.get<double>()};
	} else if (j.is_boolean()) {
		return ValuePtr{j.get<bool>()};
	} else if (j.is_object()) {
		return ValuePtr{to_obj(j)};
	} else if (j.is_array()) {
		Value::VectorType vec;
		for (const auto& elem : j) {
			vec.emplace_back(to_value(elem));
		}
		return ValuePtr{vec};
	}
	return ValuePtr::undefined;
}

ObjectType to_obj(const json& j)
{
	ObjectType obj;
	for (const auto& item : j.items()) {
		obj.set(item.key(), to_value(item.value()));
	}
	return obj;
}

}

ValuePtr import_json(const std::string& filename, const Location& loc)
{
	std::ifstream i(filename);

	try {
		if (i) {
			json j;
			i >> j;
			return ValuePtr{to_value(j)};
		} else {
			PRINTB("WARNING: Can't open import file '%s', import() at line %d", filename % loc.firstLine());
		}
	} catch (const std::exception& e) {
		PRINTB("WARNING: Failed to parse file '%s': %s, import() at line %d", filename % e.what() % loc.firstLine());
	}

	return ValuePtr::undefined;
}
/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2015 Clifford Wolf <clifford@clifford.at> and
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
 
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "export.h"
#include "expression.h"
#include "modcontext.h"

namespace pt = boost::property_tree;

static const std::string valueTypeToString(const Value::ValueType type)
{
	switch (type) {
	case Value::BOOL:
		return "BOOL";
	case Value::NUMBER:
		return "NUMBER";
	case Value::STRING:
		return "STRING";
	case Value::VECTOR:
		return "VECTOR";
	case Value::RANGE:
		return "RANGE";
	case Value::UNDEFINED:
		return "UNDEFINED";
	}
	assert(false && "unhandled value type");
}

void export_parameter(std::ostream &output, FileModule *fileModule)
{
	pt::ptree pt;
	pt::ptree parameter_array;

	ModuleContext ctx;

	BOOST_FOREACH(Assignment assignment, fileModule->scope.assignments)
	{
		const Annotation *param = assignment.annotation("Parameter");
		if (!param) {
			continue;
		}

		const ValuePtr defaultValue = assignment.second.get()->evaluate(&ctx);
		if (defaultValue->type() == Value::UNDEFINED) {
			continue;
		}

		pt::ptree parameter;
		parameter.push_back(std::make_pair("name", assignment.first));
		parameter.push_back(std::make_pair("value", defaultValue->toString()));
		parameter.push_back(std::make_pair("type", valueTypeToString(defaultValue->type())));

		const Annotation *desc = assignment.annotation("Description");
		if (desc) {
			const ValuePtr v = desc->evaluate(&ctx, "text");
			if (v->type() == Value::STRING) {
				parameter.push_back(std::make_pair("description", v->toString()));
			}
		}

		parameter_array.push_back(std::make_pair("", parameter));
	}

	pt.add_child("parameters", parameter_array);

	pt::write_json(output, pt);
}

*/

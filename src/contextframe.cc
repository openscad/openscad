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

#include "contextframe.h"

ContextFrame::ContextFrame(EvaluationSession* session):
	evaluation_session(session)
{}

boost::optional<const Value&> ContextFrame::lookup_local_variable(const std::string &name) const
{
	if (is_config_variable(name)) {
		ValueMap::const_iterator result = config_variables.find(name);
		if (result != config_variables.end()) {
			return result->second;
		}
	} else {
		ValueMap::const_iterator result = lexical_variables.find(name);
		if (result != lexical_variables.end()) {
			return result->second;
		}
	}
	return boost::none;
}

boost::optional<CallableFunction> ContextFrame::lookup_local_function(const std::string &name, const Location &loc) const
{
	boost::optional<const Value&> value = lookup_local_variable(name);
	if (value && value->type() == Value::Type::FUNCTION) {
		return CallableFunction{&*value};
	}
	return boost::none;
}

boost::optional<InstantiableModule> ContextFrame::lookup_local_module(const std::string &name, const Location &loc) const
{
	return boost::none;
}

void ContextFrame::set_variable(const std::string &name, Value&& value)
{
	if (is_config_variable(name)) {
		config_variables.insert_or_assign(name, std::move(value));
	} else {
		lexical_variables.insert_or_assign(name, std::move(value));
	}
}

void ContextFrame::apply_lexical_variables(const ContextFrame& other)
{
	lexical_variables.applyFrom(other.lexical_variables);
}

void ContextFrame::apply_config_variables(const ContextFrame& other)
{
	config_variables.applyFrom(other.config_variables);
}

void ContextFrame::apply_lexical_variables(ContextFrame&& other)
{
	lexical_variables.applyFrom(std::move(other.lexical_variables));
}

void ContextFrame::apply_config_variables(ContextFrame&& other)
{
	config_variables.applyFrom(std::move(other.config_variables));
}

void ContextFrame::apply_variables(ContextFrame&& other)
{
	lexical_variables.applyFrom(std::move(other.lexical_variables));
	config_variables.applyFrom(std::move(other.config_variables));
}

bool ContextFrame::is_config_variable(const std::string &name)
{
	return name[0] == '$' && name != "$children";
}

#ifdef DEBUG
std::string ContextFrame::dumpFrame() const
{
	std::ostringstream s;
	s << boost::format("ContextFrame %p:\n") % this;
	for(const auto &v : lexical_variables) {
		s << boost::format("    %s = %s\n") % v.first % v.second.toEchoString();
	}
	for(const auto &v : config_variables) {
		s << boost::format("    %s = %s\n") % v.first % v.second.toEchoString();
	}
	return s.str();
}
#endif

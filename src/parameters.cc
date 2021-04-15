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

#include <set>

#include "expression.h"
#include "parameters.h"

Parameters::Parameters(ContextFrame&& frame):
	frame(std::move(frame)),
	handle(&this->frame)
{}

boost::optional<const Value&> Parameters::lookup(const std::string& name) const
{
	if (ContextFrame::is_config_variable(name)) {
		return frame.session()->try_lookup_special_variable(name);
	} else {
		return frame.lookup_local_variable(name);
	}
}

const Value& Parameters::get(const std::string& name) const
{
	boost::optional<const Value&> value = lookup(name);
	if (!value) {
		return Value::undefined;
	}
	return *value;
}

double Parameters::get(const std::string& name, double default_value) const
{
	boost::optional<const Value&> value = lookup(name);
	return (value && value->type() == Value::Type::NUMBER) ? value->toDouble() : default_value;
}

const std::string& Parameters::get(const std::string& name, const std::string& default_value) const
{
	boost::optional<const Value&> value = lookup(name);
	return (value && value->type() == Value::Type::STRING) ? value->toStrUtf8Wrapper().toString() : default_value;
}

ContextFrame Parameters::to_context_frame() &&
{
	handle.release();
	return std::move(frame);
}

template<class T, class F>
static ContextFrame parse_without_defaults(
	Arguments arguments,
	const Location& loc,
	const std::vector<T>& required_parameters,
	const std::vector<T>& optional_parameters,
	bool warn_for_unexpected_arguments,
	F parameter_name
) {
	ContextFrame output{arguments.session()};
	
	std::set<std::string> named_arguments;
	
	size_t parameter_position = 0;
	bool warned_for_extra_arguments = false;
	
	for (auto& argument : arguments) {
		std::string name;
		if (argument.name) {
			name = *argument.name;
			if (named_arguments.count(name)) {
				LOG(message_group::Warning,loc,arguments.documentRoot(),"argument %1$s supplied more than once",name);
			} else if (output.lookup_local_variable(name)) {
				LOG(message_group::Warning,loc,arguments.documentRoot(),"argument %1$s overrides positional argument",name);
			} else if (warn_for_unexpected_arguments && !ContextFrame::is_config_variable(name)) {
				bool found = false;
				for (const auto& parameter : required_parameters) {
					if (parameter_name(parameter) == name) {
						found = true;
						break;
					}
				}
				for (const auto& parameter : optional_parameters) {
					if (parameter_name(parameter) == name) {
						found = true;
						break;
					}
				}
				if (!found) {
					LOG(message_group::Warning,loc,arguments.documentRoot(),"variable %1$s not specified as parameter",name);
				}
			}
			named_arguments.insert(name);
		} else {
			while (parameter_position < required_parameters.size() + optional_parameters.size()) {
				std::string candidate_name = (parameter_position < required_parameters.size())
					? parameter_name(required_parameters[parameter_position])
					: parameter_name(optional_parameters[parameter_position - required_parameters.size()])
				;
				parameter_position++;
				if (!named_arguments.count(candidate_name)) {
					name = candidate_name;
					break;
				}
			}
			if (name.empty()) {
				if (warn_for_unexpected_arguments && !warned_for_extra_arguments) {
					LOG(message_group::Warning,loc,arguments.documentRoot(),"Too many unnamed arguments supplied");
					warned_for_extra_arguments = true;
				}
				continue;
			}
		}
		
		output.set_variable(name, std::move(argument.value));
	}
	return output;
}

Parameters Parameters::parse(
	Arguments arguments,
	const Location& loc,
	const std::vector<std::string>& required_parameters,
	const std::vector<std::string>& optional_parameters
) {
	ContextFrame frame{parse_without_defaults(std::move(arguments), loc, required_parameters, optional_parameters, true,
		[](const std::string& s) -> std::string { return s; }
	)};
	
	for (const auto& parameter : required_parameters) {
		if (!frame.lookup_local_variable(parameter)) {
			frame.set_variable(parameter, Value::undefined.clone());
		}
	}
	
	return Parameters{std::move(frame)};
}

Parameters Parameters::parse(
	Arguments arguments,
	const Location& loc,
	const AssignmentList& required_parameters,
	const std::shared_ptr<const Context>& defining_context
) {
	ContextFrame frame{parse_without_defaults(std::move(arguments), loc, required_parameters, {}, OpenSCAD::parameterCheck,
		[](const std::shared_ptr<Assignment>& assignment) { return assignment->getName(); }
	)};
	
	for (const auto& parameter : required_parameters) {
		if (!frame.lookup_local_variable(parameter->getName())) {
			if (parameter->getExpr()) {
				frame.set_variable(parameter->getName(), parameter->getExpr()->evaluate(defining_context));
			} else {
				frame.set_variable(parameter->getName(), Value::undefined.clone());
			}
		}
	}
	
	return Parameters{std::move(frame)};
}

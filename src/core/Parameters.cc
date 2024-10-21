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

#include "core/Parameters.h"

#include <cassert>
#include <sstream>
#include <memory>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/Expression.h"

Parameters::Parameters(ContextFrame&& frame, Location loc) :
  loc(std::move(loc)),
  frame(std::move(frame)),
  handle(&this->frame)
{}

Parameters::Parameters(Parameters&& other) noexcept :
  loc(std::move(other.loc)),
  frame(std::move(other).to_context_frame()),
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

bool Parameters::valid(const std::string& name, const Value& value,
                       Value::Type type)
{
  if (value.type() == type) {
    return true;
  }
  print_argConvert_warning(caller, name, value, {type}, loc,
                           documentRoot());
  return false;
}

bool Parameters::valid_required(const std::string& name, Value::Type type)
{
  boost::optional<const Value&> value = lookup(name);
  if (!value) {
    LOG(message_group::Warning, loc, documentRoot(),
        "%1$s: missing argument \"%2$s\"", caller, name);
    return false;
  }
  return valid(name, *value, type);
}

bool Parameters::valid(const std::string& name, Value::Type type)
{
  boost::optional<const Value&> value = lookup(name);
  if (!value || value->isUndefined()) {
    return true;
  }
  return valid(name, *value, type);
}

// Handle all general warnings and return true if a valid number is found.
bool Parameters::validate_number(const std::string& name, double& out)
{
  boost::optional<const Value&> value = lookup(name);
  if (!value || value->isUndefined()) {
    return false;
  } else if (valid(name, *value, Value::Type::NUMBER)) {
    if (value->getFiniteDouble(out)) {
      return true;
    } else {
      LOG(message_group::Warning, loc, documentRoot(), "%1$s(..., %2$s=%3$s) argument cannot be infinite or nan", caller, name, value->toString());
      return false;
    }
  }
  return false;
}

ContextFrame Parameters::to_context_frame() &&
{
  handle.release();
  return std::move(frame);
}

template <class T, class F>
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
        LOG(message_group::Warning, loc, arguments.documentRoot(), "argument %1$s supplied more than once", name);
      } else if (output.lookup_local_variable(name)) {
        LOG(message_group::Warning, loc, arguments.documentRoot(), "argument %1$s overrides positional argument", name);
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
          LOG(message_group::Warning, loc, arguments.documentRoot(), "variable %1$s not specified as parameter", name);
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
          LOG(message_group::Warning, loc, arguments.documentRoot(), "Too many unnamed arguments supplied");
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
                                            [](const std::string& s) -> std::string {
      return s;
    }
                                            )};

  for (const auto& parameter : required_parameters) {
    if (!frame.lookup_local_variable(parameter)) {
      frame.set_variable(parameter, Value::undefined.clone());
    }
  }

  return Parameters{std::move(frame), loc};
}

Parameters Parameters::parse(
  Arguments arguments,
  const Location& loc,
  const AssignmentList& required_parameters,
  const std::shared_ptr<const Context>& defining_context
  ) {
  ContextFrame frame{parse_without_defaults(std::move(arguments), loc, required_parameters, {}, OpenSCAD::parameterCheck,
                                            [](const std::shared_ptr<Assignment>& assignment) {
      return assignment->getName();
    }
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

  return Parameters{std::move(frame), loc};
}

void Parameters::set_caller(const std::string& caller)
{
  this->caller = caller;
}

void print_argCnt_warning(
  const std::string& name,
  int found,
  const std::string& expected,
  const Location& loc,
  const std::string& documentRoot
  ) {
  LOG(message_group::Warning, loc, documentRoot, "%1$s() number of parameters does not match: expected %2$s, found %3$i", name, expected, found);
}

void print_argConvert_positioned_warning(
  const std::string& calledName,
  const std::string& where,
  const Value& found,
  std::vector<Value::Type> expected,
  const Location& loc,
  const std::string& documentRoot
  ){
  std::stringstream message;
  message << calledName << "() parameter could not be converted: " << where << ": expected ";
  if (expected.size() == 1) {
    message << Value::typeName(expected[0]);
  } else {
    assert(expected.size() > 0);
    message << "one of (" << Value::typeName(expected[0]);
    for (size_t i = 1; i < expected.size(); i++) {
      message << ", " << Value::typeName(expected[i]);
    }
    message << ")";
  }
  message << ", found " << found.typeName() << " " << "(" << found.toEchoStringNoThrow() << ")";
  LOG(message_group::Warning, loc, documentRoot, "%1$s", message.str());
}

void print_argConvert_warning(
  const std::string& calledName,
  const std::string& argName,
  const Value& found,
  std::vector<Value::Type> expected,
  const Location& loc,
  const std::string& documentRoot
  ) {
  std::stringstream message;
  message << calledName << "(..., " << argName << "=" << found.toEchoStringNoThrow() << ") Invalid type: expected ";
  if (expected.size() == 1) {
    message << Value::typeName(expected[0]);
  } else {
    assert(expected.size() > 0);
    message << "one of (" << Value::typeName(expected[0]);
    for (size_t i = 1; i < expected.size(); i++) {
      message << ", " << Value::typeName(expected[i]);
    }
    message << ")";
  }
  message << ", found " << found.typeName();
  LOG(message_group::Warning, loc, documentRoot, "%1$s", message.str());
}

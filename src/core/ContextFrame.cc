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

#include "core/ContextFrame.h"

#include <utility>
#include <cstddef>
#include <string>
#include <vector>


ContextFrame::ContextFrame(EvaluationSession *session) :
  evaluation_session(session)
{}

boost::optional<const Value&> ContextFrame::lookup_local_variable(const std::string& name) const
{
  if (is_config_variable(name)) {
    auto result = config_variables.find(name);
    if (result != config_variables.end()) {
      return result->second;
    }
  } else {
    auto result = lexical_variables.find(name);
    if (result != lexical_variables.end()) {
      return result->second;
    }
  }
  return boost::none;
}

boost::optional<CallableFunction> ContextFrame::lookup_local_function(const std::string& name, const Location& /*loc*/) const
{
  boost::optional<const Value&> value = lookup_local_variable(name);
  if (value && value->type() == Value::Type::FUNCTION) {
    return CallableFunction{&*value};
  }
  return boost::none;
}

boost::optional<InstantiableModule> ContextFrame::lookup_local_module(const std::string& /*name*/, const Location& /*loc*/) const
{
  return boost::none;
}

std::vector<const Value *> ContextFrame::list_embedded_values() const
{
  std::vector<const Value *> output;
  for (const auto& variable : lexical_variables) {
    output.push_back(&variable.second);
  }
  for (const auto& variable : config_variables) {
    output.push_back(&variable.second);
  }
  return output;
}

size_t ContextFrame::clear()
{
  size_t removed = lexical_variables.size() + config_variables.size();
  lexical_variables.clear();
  config_variables.clear();
  return removed;
}

bool ContextFrame::set_variable(const std::string& name, Value&& value)
{
  if (is_config_variable(name)) {
    return config_variables.insert_or_assign(name, std::move(value)).second;
  } else {
    return lexical_variables.insert_or_assign(name, std::move(value)).second;
  }
}

void ContextFrame::apply_variables(const ValueMap& variables)
{
  for (const auto& variable : variables) {
    set_variable(variable.first, variable.second.clone());
  }
}

void ContextFrame::apply_lexical_variables(const ContextFrame& other)
{
  apply_variables(other.lexical_variables);
}

void ContextFrame::apply_config_variables(const ContextFrame& other)
{
  apply_variables(other.config_variables);
}

void ContextFrame::apply_variables(ValueMap&& variables)
{
  for (auto& variable : variables) {
    set_variable(variable.first, std::move(variable.second));
  }
  variables.clear();
}

void ContextFrame::apply_lexical_variables(ContextFrame&& other)
{
  apply_variables(std::move(other.lexical_variables));
}

void ContextFrame::apply_config_variables(ContextFrame&& other)
{
  apply_variables(std::move(other.config_variables));
}

void ContextFrame::apply_variables(ContextFrame&& other)
{
  apply_variables(std::move(other.lexical_variables));
  apply_variables(std::move(other.config_variables));
}

bool ContextFrame::is_config_variable(const std::string& name)
{
  return name[0] == '$' && name != "$children";
}

#ifdef DEBUG
std::string ContextFrame::dumpFrame() const
{
  std::ostringstream s;
  s << boost::format("ContextFrame %p:\n") % this;
  for (const auto& v : lexical_variables) {
    s << boost::format("    %s = %s\n") % v.first % v.second.toEchoString();
  }
  for (const auto& v : config_variables) {
    s << boost::format("    %s = %s\n") % v.first % v.second.toEchoString();
  }
  return s.str();
}
#endif // ifdef DEBUG

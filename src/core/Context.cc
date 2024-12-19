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

#include "core/Context.h"

#include <utility>
#include <memory>
#include <cstddef>
#include <string>
#include <vector>

#include "core/function.h"
#include "utils/printutils.h"

Context::Context(EvaluationSession *session) :
  ContextFrame(session),
  parent(nullptr)
{}

Context::Context(const std::shared_ptr<const Context>& parent) :
  ContextFrame(parent->evaluation_session),
  parent(parent)
{}

Context::~Context()
{
  Context::clear();
  if (accountingAdded)   // avoiding bad accounting where exception threw in constructor issue #3871
    session()->contextMemoryManager().releaseContext();
}

const Children *Context::user_module_children() const
{
  if (parent) {
    return parent->user_module_children();
  } else {
    return nullptr;
  }
}

std::vector<const std::shared_ptr<const Context> *> Context::list_referenced_contexts() const
{
  std::vector<const std::shared_ptr<const Context> *> output;
  if (parent) {
    output.push_back(&parent);
  }
  return output;
}

boost::optional<const Value&> Context::try_lookup_variable(const std::string& name) const
{
  if (is_config_variable(name)) {
    return session()->try_lookup_special_variable(name);
  }
  for (const Context *context = this; context != nullptr; context = context->getParent().get()) {
    boost::optional<const Value&> result = context->lookup_local_variable(name);
    if (result) {
      return result;
    }
  }
  return boost::none;
}

const Value& Context::lookup_variable(const std::string& name, const Location& loc) const
{
  boost::optional<const Value&> result = try_lookup_variable(name);
  if (!result) {
    LOG(message_group::Warning, loc, documentRoot(), "Ignoring unknown variable '%1$s'", name);
    return Value::undefined;
  }
  return *result;
}

boost::optional<CallableFunction> Context::lookup_function(const std::string& name, const Location& loc) const
{
  if (is_config_variable(name)) {
    return session()->lookup_special_function(name, loc);
  }
  for (const Context *context = this; context != nullptr; context = context->getParent().get()) {
    boost::optional<CallableFunction> result = context->lookup_local_function(name, loc);
    if (result) {
      return result;
    }
  }
  LOG(message_group::Warning, loc, documentRoot(), "Ignoring unknown function '%1$s'", name);
  return boost::none;
}

boost::optional<InstantiableModule> Context::lookup_module(const std::string& name, const Location& loc) const
{
  if (is_config_variable(name)) {
    return session()->lookup_special_module(name, loc);
  }
  for (const Context *context = this; context != nullptr; context = context->getParent().get()) {
    boost::optional<InstantiableModule> result = context->lookup_local_module(name, loc);
    if (result) {
      return result;
    }
  }
  LOG(message_group::Warning, loc, this->documentRoot(), "Ignoring unknown module '%1$s'", name);
  return boost::none;
}

bool Context::set_variable(const std::string& name, Value&& value)
{
  bool new_variable = ContextFrame::set_variable(name, std::move(value));
  if (new_variable) {
    session()->accounting().addContextVariable();
  }
  return new_variable;
}

size_t Context::clear()
{
  size_t removed = ContextFrame::clear();
  session()->accounting().removeContextVariable(removed);
  return removed;
}

#ifdef DEBUG
std::string Context::dump() const
{
  std::ostringstream s;
  s << boost::format("Context %p:\n") % this;
  Context const *context = this;
  while (context) {
    s << "  " << context->dumpFrame();
    context = context->getParent().get();
  }
  return s.str();
}
#endif // ifdef DEBUG

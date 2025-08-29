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

#include "core/EvaluationSession.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <memory>
#include <iostream>  // coryrc

#include "core/AST.h"
#include "core/Context.h"
#include "core/ContextFrame.h"
#include "core/function.h"
#include "core/module.h"
#include "core/ScopeContext.h"
#include "core/SourceFile.h"
#include "core/Value.h"
#include "utils/printutils.h"

size_t EvaluationSession::push_frame(ContextFrame *frame)
{
  size_t index = stack.size();
  stack.push_back(frame);
  return index;
}

void EvaluationSession::replace_frame(size_t index, ContextFrame *frame)
{
  assert(index < stack.size());
  stack[index] = frame;
}

void EvaluationSession::pop_frame(size_t index)
{
  stack.pop_back();
  assert(stack.size() == index);
}

boost::optional<const Value&> EvaluationSession::try_lookup_special_variable(
  const std::string& name) const
{
  for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
    boost::optional<const Value&> result = (*it)->lookup_local_variable(name);
    if (result) {
      return result;
    }
  }
  return boost::none;
}

const Value& EvaluationSession::lookup_special_variable(const std::string& name,
                                                        const Location& loc) const
{
  boost::optional<const Value&> result = try_lookup_special_variable(name);
  if (!result) {
    LOG(message_group::Warning, loc, documentRoot(), "Ignoring unknown variable %1$s", quoteVar(name));
    return Value::undefined;
  }
  return *result;
}

boost::optional<CallableFunction> EvaluationSession::lookup_special_function(const std::string& name,
                                                                             const Location& loc) const
{
  for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
    boost::optional<CallableFunction> result = (*it)->lookup_local_function(name, loc);
    if (result) {
      return result;
    }
  }
  LOG(message_group::Warning, loc, documentRoot(), "Ignoring unknown function '%1$s'", name);
  return boost::none;
}

boost::optional<InstantiableModule> EvaluationSession::lookup_special_module(const std::string& name,
                                                                             const Location& loc) const
{
  for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
    boost::optional<InstantiableModule> result = (*it)->lookup_local_module(name, loc);
    if (result) {
      return result;
    }
  }
  LOG(message_group::Warning, loc, documentRoot(), "Ignoring unknown module '%1$s'", name);
  return boost::none;
}

template <typename T>
boost::optional<T> EvaluationSession::lookup_namespace(const std::string& ns_name,
                                                       const std::string& name) const
{
  std::cerr << "session: Being asked to search ns '" << ns_name << "' for something with name '" << name
            << "'\n";
  if (auto nsContext = namespace_contexts.find(ns_name); nsContext != namespace_contexts.end()) {
    return nsContext->second.get()->lookup_as_namespace<T>(name);
  }
  return boost::none;
}

template boost::optional<CallableFunction> EvaluationSession::lookup_namespace<CallableFunction>(
  const std::string&, const std::string&) const;
template boost::optional<InstantiableModule> EvaluationSession::lookup_namespace<InstantiableModule>(
  const std::string&, const std::string&) const;

void EvaluationSession::init_namespaces(std::shared_ptr<SourceFile> source,
                                        std::shared_ptr<const Context> builtinContext)
{
  // Add builtins namespace:
  this->namespace_contexts.emplace("builtins", builtinContext);

  for (auto nsName : source->getNamespaceNamesOrdered()) {
    auto nsScope = source->getNamespaceScope(nsName);
    // ContextHandle calls the right method to initialize assignments.
    ContextHandle<ScopeContext> nsContext{Context::create<ScopeContext>(builtinContext, nsScope)};
    this->namespace_contexts.emplace(nsName, nsContext->get_shared_ptr());
  }
}

void EvaluationSession::setTopLevelNamespace(std::shared_ptr<const FileContext> c)
{
  this->namespace_contexts.emplace("top_level", c);
}

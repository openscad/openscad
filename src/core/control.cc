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

#include <utility>
#include <memory>
#include <cstddef>
#include <vector>

#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/node.h"
#include "core/Arguments.h"
#include "core/Children.h"
#include "core/Expression.h"
#include "core/Builtins.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include <cstdint>

static std::shared_ptr<AbstractNode> lazyUnionNode(const ModuleInstantiation *inst)
{
  if (Feature::ExperimentalLazyUnion.is_enabled()) {
    return std::make_shared<ListNode>(inst);
  } else {
    return std::make_shared<GroupNode>(inst);
  }
}

static boost::optional<size_t> validChildIndex(int n, const Children *children, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  if (n < 0 || n >= static_cast<int>(children->size())) {
    LOG(message_group::Warning, inst->location(), context->documentRoot(), "Children index (%1$d) out of bounds (%2$d children)", n, children->size());
    return boost::none;
  }
  return size_t(n);
}

static boost::optional<size_t> validChildIndex(const Value& value, const Children *children, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  if (value.type() != Value::Type::NUMBER) {
    LOG(message_group::Warning, inst->location(), context->documentRoot(), "Bad parameter type (%1$s) for children, only accept: empty, number, vector, range.", value.toString());
    return boost::none;
  }
  return validChildIndex(static_cast<int>(value.toDouble()), children, inst, context);
}

static std::shared_ptr<AbstractNode> builtin_child(const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  LOG(message_group::Deprecated, "child() will be removed in future releases. Use children() instead.");

  if (!inst->scope.moduleInstantiations.empty()) {
    LOG(message_group::Warning, inst->location(), context->documentRoot(),
        "module %1$s() does not support child modules", inst->name());
  }

  Arguments arguments{inst->arguments, context};
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {}, std::vector<std::string>{"index"});
  const Children *children = context->user_module_children();
  if (!children) {
    // child() called outside any user module
    return nullptr;
  }

  boost::optional<size_t> index;
  if (!parameters.contains("index")) {
    index = validChildIndex(0, children, inst, context);
  } else {
    index = validChildIndex(parameters["index"], children, inst, context);
  }
  if (!index) {
    return nullptr;
  }
  return children->instantiate(lazyUnionNode(inst), {*index});
}

static std::shared_ptr<AbstractNode> builtin_children(const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  if (!inst->scope.moduleInstantiations.empty()) {
    LOG(message_group::Warning, inst->location(), context->documentRoot(),
        "module %1$s() does not support child modules", inst->name());
  }

  Arguments arguments{inst->arguments, context};
  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(), {}, std::vector<std::string>{"index"});
  const Children *children = context->user_module_children();
  if (!children) {
    // children() called outside any user module
    return nullptr;
  }

  if (!parameters.contains("index")) {
    // no arguments => all children
    return children->instantiate(lazyUnionNode(inst));
  }

  // one (or more ignored) argument
  if (parameters["index"].type() == Value::Type::NUMBER) {
    auto index = validChildIndex(parameters["index"], children, inst, context);
    if (!index) {
      return nullptr;
    }
    return children->instantiate(lazyUnionNode(inst), {*index});
  } else if (parameters["index"].type() == Value::Type::VECTOR) {
    std::vector<size_t> indices;
    for (const auto& val : parameters["index"].toVector()) {
      auto index = validChildIndex(val, children, inst, context);
      if (index) {
        indices.push_back(*index);
      }
    }
    return children->instantiate(lazyUnionNode(inst), indices);
  } else if (parameters["index"].type() == Value::Type::RANGE) {
    const RangeType& range = parameters["index"].toRange();
    uint32_t steps = range.numValues();
    if (steps >= RangeType::MAX_RANGE_STEPS) {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Bad range parameter for children: too many elements (%1$lu)", steps);
      return nullptr;
    }
    std::vector<size_t> indices;
    for (double d : range) {
      auto index = validChildIndex(static_cast<int>(d), children, inst, context);
      if (index) {
        indices.push_back(*index);
      }
    }
    return children->instantiate(lazyUnionNode(inst), indices);
  } else {
    // Invalid argument
    LOG(message_group::Warning, inst->location(), parameters.documentRoot(), "Bad parameter type (%1$s) for children, only accept: empty, number, vector, range", parameters["index"].toEchoStringNoThrow());
    return {};
  }
}

static std::shared_ptr<AbstractNode> builtin_echo(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  LOG(message_group::Echo, "%1$s", STR(arguments));

  auto node = children.instantiate(lazyUnionNode(inst));
  // echo without child geometries should not count as valid CSGNode
  if (node->children.empty()) {
    return {};
  }
  return node;
}

static std::shared_ptr<AbstractNode> builtin_assert(const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  Assert::performAssert(inst->arguments, inst->location(), context);

  auto node = Children(&inst->scope, context).instantiate(lazyUnionNode(inst));
  // assert without child geometries should not count as valid CSGNode
  if (node->children.empty()) {
    return {};
  }
  return node;
}

static std::shared_ptr<AbstractNode> builtin_let(const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  return Children(&inst->scope, *Let::sequentialAssignmentContext(inst->arguments, inst->location(), context)).instantiate(lazyUnionNode(inst));
}

static std::shared_ptr<AbstractNode> builtin_assign(const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  // We create a new context to avoid arguments from influencing each other
  // -> parallel evaluation. This is to be backwards compatible.
  Arguments arguments{inst->arguments, context};
  ContextHandle<Context> assignContext{Context::create<Context>(context)};
  for (auto& argument : arguments) {
    if (!argument.name) {
      LOG(message_group::Warning, inst->location(), context->documentRoot(), "Assignment without variable name %1$s", argument->toEchoStringNoThrow());
    } else {
      if (assignContext->lookup_local_variable(*argument.name)) {
        LOG(message_group::Warning, inst->location(), context->documentRoot(), "Duplicate variable assignment %1$s = %2$s", *argument.name, argument->toEchoStringNoThrow());
      }
      assignContext->set_variable(*argument.name, std::move(argument.value));
    }
  }

  return Children(&inst->scope, *assignContext).instantiate(lazyUnionNode(inst));
}

static std::shared_ptr<AbstractNode> builtin_for(const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  auto node = lazyUnionNode(inst);
  if (!inst->arguments.empty()) {
    LcFor::forEach(inst->arguments, inst->location(), context,
                   [inst, node] (const std::shared_ptr<const Context>& iterationContext) {
      Children(&inst->scope, iterationContext).instantiate(node);
    }
                   );
  }
  return node;
}

static std::shared_ptr<AbstractNode> builtin_intersection_for(const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  auto node = std::make_shared<AbstractIntersectionNode>(inst);
  if (!inst->arguments.empty()) {
    LcFor::forEach(inst->arguments, inst->location(), context,
                   [inst, node] (const std::shared_ptr<const Context>& iterationContext) {
      Children(&inst->scope, iterationContext).instantiate(node);
    }
                   );
  }
  return node;
}

static std::shared_ptr<AbstractNode> builtin_if(const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context)
{
  Arguments arguments{inst->arguments, context};
  const auto *ifelse = dynamic_cast<const IfElseModuleInstantiation *>(inst);
  if (arguments.size() > 0 && arguments[0]->toBool()) {
    return Children(&inst->scope, context).instantiate(lazyUnionNode(inst));
  } else if (ifelse->getElseScope()) {
    return Children(ifelse->getElseScope(), context).instantiate(lazyUnionNode(inst));
  } else {
    // "if" with failed condition, and no "else" should not count as valid CSGNode
    return nullptr;
  }
}

void register_builtin_control()
{
  Builtins::init("assign", new BuiltinModule(builtin_assign));
  Builtins::init("child", new BuiltinModule(builtin_child));

  Builtins::init("children", new BuiltinModule(builtin_children),
  {
    "children()",
    "children(number)",
    "children([start : step : end])",
    "children([start : end])",
    "children([vector])",
  });

  Builtins::init("echo", new BuiltinModule(builtin_echo),
  {
    "echo(arg, ...)",
  });

  Builtins::init("assert", new BuiltinModule(builtin_assert),
  {
    "assert(boolean)",
    "assert(boolean, string)",
  });

  Builtins::init("for", new BuiltinModule(builtin_for),
  {
    "for([start : increment : end])",
    "for([start : end])",
    "for([vector])",
  });

  Builtins::init("let", new BuiltinModule(builtin_let),
  {
    "let(arg, ...) expression",
  });

  Builtins::init("intersection_for", new BuiltinModule(builtin_intersection_for),
  {
    "intersection_for([start : increment : end])",
    "intersection_for([start : end])",
    "intersection_for([vector])",
  });

  Builtins::init("if", new BuiltinModule(builtin_if),
  {
    "if(boolean)",
  });
}

#pragma once

#include <utility>

#include "Context.h"
#include "LocalScope.h"

class AbstractNode;
class ScopeContext;

class Children
{
public:
  Children(const LocalScope *children_scope, std::shared_ptr<const Context> context) :
    children_scope(children_scope),
    context(std::move(context))
  {}

  Children(Children&& other) = default;
  Children& operator=(Children&& other) = default;
  Children(const Children& other) = default;
  Children& operator=(const Children& other) = default;
  ~Children() = default;

  [[nodiscard]] std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<AbstractNode>& target) const;
  [[nodiscard]] std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<AbstractNode>& target, const std::vector<size_t>& indices) const;

  [[nodiscard]] bool empty() const { return !children_scope->hasChildren(); }
  [[nodiscard]] size_t size() const { return children_scope->moduleInstantiations.size(); }
  [[nodiscard]] const std::shared_ptr<const Context>& getContext() const { return context; }

private:
  const LocalScope *children_scope;
  std::shared_ptr<const Context> context;

  [[nodiscard]] ContextHandle<ScopeContext> scopeContext() const;
};

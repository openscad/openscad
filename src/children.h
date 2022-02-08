#pragma once

#include "context.h"
#include "localscope.h"

class AbstractNode;
class ScopeContext;

class Children
{
public:
  Children(const LocalScope *children_scope, const std::shared_ptr<const Context>& context) :
    children_scope(children_scope),
    context(context)
  {}

  Children(Children&& other) = default;
  Children& operator=(Children&& other) = default;

  std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<AbstractNode> &target) const;
  std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<AbstractNode> &target, const std::vector<size_t>& indices) const;

  bool empty() const { return !children_scope->hasChildren(); }
  size_t size() const { return children_scope->moduleInstantiations.size(); }
  const std::shared_ptr<const Context>& getContext() const { return context; }

private:
  const LocalScope *children_scope;
  std::shared_ptr<const Context> context;

  ContextHandle<ScopeContext> scopeContext() const;
};

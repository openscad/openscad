#pragma once

#include <cstddef>
#include <utility>
#include <memory>
#include <vector>

class AbstractNode;
class ScopeContext;
template <typename T>
class ContextHandle;
class Context;
class LocalScope;

class Children
{
public:
  Children(std::shared_ptr<const LocalScope> children_scope, std::shared_ptr<const Context> context)
    : children_scope(std::move(children_scope)), context(std::move(context))
  {
  }

  Children(Children&& other) = default;
  Children& operator=(Children&& other) = default;
  Children(const Children& other) = default;
  Children& operator=(const Children& other) = default;
  ~Children() = default;

  // NOLINTBEGIN(modernize-use-nodiscard)
  // instantiate just returns a copy of target shared_ptr as a convenience, not crucial to use this value
  std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<AbstractNode>& target) const;
  std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<AbstractNode>& target,
                                            const std::vector<size_t>& indices) const;
  // NOLINTEND(modernize-use-nodiscard)

  bool empty() const;
  size_t size() const;
  [[nodiscard]] const std::shared_ptr<const Context>& getContext() const { return context; }

private:
  std::shared_ptr<const LocalScope> children_scope;
  std::shared_ptr<const Context> context;

  [[nodiscard]] ContextHandle<ScopeContext> scopeContext() const;
};

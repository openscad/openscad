#pragma once

#include <cassert>
#include <memory>
#include <cstddef>
#include <string>
#include <vector>

#include "core/ContextFrame.h"
#include "core/ContextMemoryManager.h"

/**
 * Local handle to a all context objects. This is used to maintain the
 * dynamic scoping stack using object lifetime.
 * The Context objects can hang around for longer, e.g. in case of closures.
 */
template <typename T>
class ContextHandle : ContextFrameHandle
{
public:
  ContextHandle(std::shared_ptr<T>&& context) :
    ContextFrameHandle(context.get()),
    context(std::move(context))
  {
    try {
      this->context->init();
    } catch (...) {
      session->contextMemoryManager().addContext(std::move(this->context));
      throw;
    }
  }

  ~ContextHandle()
  {
    assert(!!session == !!context);
    if (session) {
      session->contextMemoryManager().addContext(std::move(this->context));
    }
  }

  ContextHandle(const ContextHandle&) = delete;
  ContextHandle& operator=(const ContextHandle&) = delete;
  ContextHandle(ContextHandle&& other) noexcept = default;

  // Valid only if $other is on the top of the stack.
  ContextHandle& operator=(ContextHandle&& other) noexcept {
    assert(session);
    assert(context);
    assert(other.context);
    assert(other.session);

    //session->contextMemoryManager().releaseContext();
    session->contextMemoryManager().addContext(std::move(this->context));
    other.release();
    context = std::move(other.context);
    ContextFrameHandle::operator=(context.get());
    return *this;
  }

  const T *operator->() const { return context.get(); }
  T *operator->() { return context.get(); }
  std::shared_ptr<const T> operator*() const { return context; }

private:
  std::shared_ptr<T> context;
};

class Context : public ContextFrame, public std::enable_shared_from_this<Context>
{
protected:
  Context(EvaluationSession *session);
  Context(const std::shared_ptr<const Context>& parent);

public:
  ~Context() override;

  template <typename C, typename ... T>
  static ContextHandle<C> create(T&& ... t) {
    return ContextHandle<C>{std::shared_ptr<C>(new C(std::forward<T>(t)...))};
  }

  virtual void init() { }

  std::shared_ptr<const Context> get_shared_ptr() const { return shared_from_this(); }
  virtual const class Children *user_module_children() const;
  virtual std::vector<const std::shared_ptr<const Context> *> list_referenced_contexts() const;

  boost::optional<const Value&> try_lookup_variable(const std::string& name) const;
  const Value& lookup_variable(const std::string& name, const Location& loc) const;
  boost::optional<CallableFunction> lookup_function(const std::string& name, const Location& loc) const;
  boost::optional<InstantiableModule> lookup_module(const std::string& name, const Location& loc) const;
  bool set_variable(const std::string& name, Value&& value) override;
  size_t clear() override;

  const std::shared_ptr<const Context>& getParent() const { return this->parent; }
  // This modifies the semantics of the context in an error-prone way. Use with caution.
  void setParent(const std::shared_ptr<const Context>& parent) { this->parent = parent; }

  void setAccountingAdded() { accountingAdded = true; }

protected:
  std::shared_ptr<const Context> parent;

  bool accountingAdded = false;   // avoiding bad accounting when exception threw in constructor issue #3871

public:
#ifdef DEBUG
  std::string dump() const;
#endif
};

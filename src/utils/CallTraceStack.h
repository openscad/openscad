#pragma once

/**
 * CallTraceStack - A thread-local call stack for collecting trace information
 * without using expensive catch-rethrow exception handling.
 *
 * The Problem:
 * MSVC exception handling uses significant stack space (~17KB per level) when
 * unwinding through catch-rethrow patterns. With deep recursion (1000+ levels),
 * this causes stack overflow during the unwind phase.
 *
 * The Solution:
 * Use RAII guards to track the call stack. When an exception is thrown, it
 * propagates directly without being caught at intermediate levels. The trace
 * is collected from the CallTraceStack at the top-level catch site.
 *
 * Usage:
 *   // In recursive functions/modules - use Guard instead of try/catch
 *   void recursive_function() {
 *     CallTraceStack::Guard trace_guard("function_name", location, context);
 *     // ... do work, call recursively ...
 *   }  // Guard automatically pops on scope exit
 *
 *   // At top level catch site:
 *   try {
 *     evaluate();
 *   } catch (EvaluationException& e) {
 *     e.printCallTrace();  // Print all accumulated trace
 *     CallTraceStack::clear();
 *   }
 */

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <exception>
#include "core/AST.h"

// Forward declaration - Context is defined in core/Context.h
class Context;

class CallTraceStack
{
public:
  // Information stored for each call frame
  struct Entry {
    enum class Type { FunctionCall, ModuleInstantiation, UserModuleCall, Assignment };

    Type type;
    std::string name;
    Location location;
    std::weak_ptr<const Context> context;

    // Optional: for Assignment entries, store overwrite location if present
    Location overwriteLocation;

    // Optional: for UserModule, store pre-computed parameter string
    // Must be computed eagerly while context is still valid
    std::string parameterString;

    Entry(Type t, std::string n, Location loc, std::shared_ptr<const Context> ctx)
      : type(t),
        name(std::move(n)),
        location(std::move(loc)),
        context(ctx),
        overwriteLocation(Location::NONE)
    {
    }
  };

  // Get the thread-local stack
  static std::vector<Entry>& getStack()
  {
    static thread_local std::vector<Entry> stack;
    return stack;
  }

  // Push an entry
  static void push(Entry entry) { getStack().push_back(std::move(entry)); }

  // Pop an entry
  static void pop()
  {
    auto& stack = getStack();
    if (!stack.empty()) {
      stack.pop_back();
    }
  }

  // Get current depth
  static size_t depth() { return getStack().size(); }

  // Clear the stack
  static void clear() { getStack().clear(); }

  // RAII Guard for automatic push/pop
  class Guard
  {
  public:
    Guard(Entry::Type type, const std::string& name, const Location& loc,
          const std::shared_ptr<const Context>& ctx)
      : active_(true), uncaught_on_entry_(std::uncaught_exceptions())
    {
      CallTraceStack::push(Entry(type, name, loc, ctx));
    }

    // Allow setting parameter string (for UserModule)
    // Must be called immediately while context is valid
    void setParameterString(std::string params)
    {
      auto& stack = CallTraceStack::getStack();
      if (!stack.empty()) {
        stack.back().parameterString = std::move(params);
      }
    }

    ~Guard()
    {
      if (active_) {
        // Only pop if we're NOT unwinding due to an exception thrown after we were created
        // If std::uncaught_exceptions() is greater than when we were created,
        // an exception is propagating and we should preserve the trace
        if (std::uncaught_exceptions() <= uncaught_on_entry_) {
          CallTraceStack::pop();
        }
      }
    }

    // Non-copyable
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
    // Move constructor - transfers ownership
    Guard(Guard&& other) noexcept : active_(other.active_), uncaught_on_entry_(other.uncaught_on_entry_)
    {
      other.active_ = false;  // Prevent other from popping
    }
    Guard& operator=(Guard&&) = delete;

  private:
    bool active_;
    int uncaught_on_entry_;  // Value of std::uncaught_exceptions() when Guard was created
  };

  // Convenience factory methods for guards
  static Guard functionCall(const std::string& name, const Location& loc,
                            const std::shared_ptr<const Context>& ctx)
  {
    return Guard(Entry::Type::FunctionCall, name, loc, ctx);
  }

  static Guard moduleInstantiation(const std::string& name, const Location& loc,
                                   const std::shared_ptr<const Context>& ctx)
  {
    return Guard(Entry::Type::ModuleInstantiation, name, loc, ctx);
  }

  static Guard userModuleCall(const std::string& name, const Location& loc,
                              const std::shared_ptr<const Context>& ctx)
  {
    return Guard(Entry::Type::UserModuleCall, name, loc, ctx);
  }

  static Guard assignment(const std::string& varName, const Location& loc,
                          const std::shared_ptr<const Context>& ctx,
                          const Location& overwriteLoc = Location::NONE)
  {
    Guard g(Entry::Type::Assignment, varName, loc, ctx);
    auto& stack = getStack();
    if (!stack.empty()) {
      stack.back().overwriteLocation = overwriteLoc;
    }
    return g;
  }
};

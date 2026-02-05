#pragma once

#include <boost/optional.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/AST.h"
#include "core/ContextMemoryManager.h"  // FIXME: don't use as value type so we don't need to include header
#include "core/callables.h"

class Value;
class ContextFrame;

class EvaluationSession
{
public:
  EvaluationSession(std::string documentRoot);
  ~EvaluationSession();

  enum class TimerType {
    Monotonic,
    Cpu,
  };

  size_t push_frame(ContextFrame *frame);
  void replace_frame(size_t index, ContextFrame *frame);
  void pop_frame(size_t index);

  [[nodiscard]] boost::optional<const Value&> try_lookup_special_variable(const std::string& name) const;
  [[nodiscard]] const Value& lookup_special_variable(const std::string& name, const Location& loc) const;
  [[nodiscard]] boost::optional<CallableFunction> lookup_special_function(const std::string& name,
                                                                          const Location& loc) const;
  [[nodiscard]] boost::optional<InstantiableModule> lookup_special_module(const std::string& name,
                                                                          const Location& loc) const;

  [[nodiscard]] const std::string& documentRoot() const { return document_root; }
  ContextMemoryManager& contextMemoryManager() { return context_memory_manager; }
  HeapSizeAccounting& accounting() { return context_memory_manager.accounting(); }

  int timer_new(const std::string& name, TimerType type);
  void timer_start(int id, const Location& loc);
  void timer_clear(int id, const Location& loc);
  double timer_stop(int id, const Location& loc);
  double timer_elapsed(int id, const Location& loc);
  void timer_delete(int id, const Location& loc);
  const std::string& timer_name(int id, const Location& loc) const;

private:
  struct TimerRegistry;

  std::string document_root;
  std::vector<ContextFrame *> stack;
  ContextMemoryManager context_memory_manager;
  std::unique_ptr<TimerRegistry> timer_registry;
};

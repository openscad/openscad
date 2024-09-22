#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "core/ContextMemoryManager.h"
#include "core/function.h"
#include "core/module.h"
#include "core/Value.h"

class ContextFrame;

class EvaluationSession
{
public:
  EvaluationSession(std::string documentRoot) :
    document_root(std::move(documentRoot))
  {}

  size_t push_frame(ContextFrame *frame);
  void replace_frame(size_t index, ContextFrame *frame);
  void pop_frame(size_t index);

  [[nodiscard]] boost::optional<const Value&> try_lookup_special_variable(const std::string& name) const;
  [[nodiscard]] const Value& lookup_special_variable(const std::string& name, const Location& loc) const;
  [[nodiscard]] boost::optional<CallableFunction> lookup_special_function(const std::string& name, const Location& loc) const;
  [[nodiscard]] boost::optional<InstantiableModule> lookup_special_module(const std::string& name, const Location& loc) const;

  [[nodiscard]] const std::string& documentRoot() const { return document_root; }
  ContextMemoryManager& contextMemoryManager() { return context_memory_manager; }
  HeapSizeAccounting& accounting() { return context_memory_manager.accounting(); }

private:
  std::string document_root;
  std::vector<ContextFrame *> stack;
  ContextMemoryManager context_memory_manager;
};

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "core/callables.h"
#include "core/ContextMemoryManager.h"  // FIXME: don't use as value type so we don't need to include header

class BuiltinContext;
class ContextFrame;
class ScopeContext;
class SourceFile;
class FileContext;
class Value;

class EvaluationSession
{
public:
  EvaluationSession(std::string documentRoot) : document_root(std::move(documentRoot)) {}

  size_t push_frame(ContextFrame *frame);
  void replace_frame(size_t index, ContextFrame *frame);
  void pop_frame(size_t index);

  [[nodiscard]] boost::optional<const Value&> try_lookup_special_variable(const std::string& name) const;
  [[nodiscard]] const Value& lookup_special_variable(const std::string& name, const Location& loc) const;
  [[nodiscard]] boost::optional<CallableFunction> lookup_special_function(const std::string& name,
                                                                          const Location& loc) const;
  [[nodiscard]] boost::optional<InstantiableModule> lookup_special_module(const std::string& name,
                                                                          const Location& loc) const;

  /**
   * @brief Lookup something from a namespace's environments
   *
   * Use these for looking up functions or modules from a namespace.
   * TODO: coryrc - add assignments
   */
  template <typename T>
  boost::optional<T> lookup_namespace(const std::string& ns_name, const std::string& name) const;

  void init_namespaces(std::shared_ptr<SourceFile> source,
                       std::shared_ptr<const Context> builtinContext);
  void setTopLevelNamespace(std::shared_ptr<const FileContext> c);

  [[nodiscard]] const std::string& documentRoot() const { return document_root; }
  ContextMemoryManager& contextMemoryManager() { return context_memory_manager; }
  HeapSizeAccounting& accounting() { return context_memory_manager.accounting(); }

private:
  std::string document_root;
  std::vector<ContextFrame *> stack;
  ContextMemoryManager context_memory_manager;
  std::unordered_map<std::string, std::shared_ptr<const Context>> namespace_contexts;
};

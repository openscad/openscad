#pragma once

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>
#include <boost/optional.hpp>

#include "core/AST.h"
#include "core/callables.h"
#include "core/ValueMap.h"
#include "core/function.h"
#include "core/module.h"

class EvaluationSession;

class EvaluationSession;
class Value;

class EvaluationSession;
class Value;

class ContextFrame
{
public:
  ContextFrame(EvaluationSession *session);
  virtual ~ContextFrame();

  ContextFrame(ContextFrame&& other) = default;

  virtual boost::optional<const Value&> lookup_local_variable(const std::string& name) const;
  virtual boost::optional<CallableFunction> lookup_local_function(const std::string& name,
                                                                  const Location& loc) const;
  virtual boost::optional<InstantiableModule> lookup_local_module(const std::string& name,
                                                                  const Location& loc) const;

  /**
   * @brief Lookup as if referencing by namespace
   *
   * Use this for looking up functions or modules from this context when
   * the name is referenced via `using` or directly `ns::name`.
   * Will search assignments for function literals.
   * Does not follow `using` imports.
   *
   * TODO: github.com/openscad/openscad/issues/4724 - When adding module literals, lookup in assignments.
   */
  template <typename T>
  boost::optional<T> lookup_as_namespace(const std::string& name) const;
  virtual boost::optional<CallableFunction> lookup_function_as_namespace(const std::string& name) const;
  virtual boost::optional<InstantiableModule> lookup_module_as_namespace(const std::string& name) const;
  virtual boost::optional<const Value&> lookup_variable_as_namespace(const std::string& name) const;

  /**
   * @brief If a namespace context, is the name; otherwise empty string.
   */
  virtual const std::string get_namespace_name() const { return {}; }

  virtual std::vector<const Value *> list_embedded_values() const;
  virtual size_t clear();

  virtual bool set_variable(const std::string& name, Value&& value);

  void apply_variables(const ValueMap& variables);
  void apply_lexical_variables(const ContextFrame& other);
  void apply_config_variables(const ContextFrame& other);
  void apply_variables(const ContextFrame& other)
  {
    apply_lexical_variables(other);
    apply_config_variables(other);
  }

  void apply_variables(ValueMap&& variables);
  void apply_variables(std::unique_ptr<ContextFrame>&& other);

  static bool is_config_variable(const std::string& name);

  EvaluationSession *session() const { return evaluation_session; }
  const std::string& documentRoot() const;

protected:
  ValueMap lexical_variables;
  ValueMap config_variables;
  EvaluationSession *evaluation_session;

public:
#ifdef DEBUG
  virtual std::string dumpFrame() const;
#endif
};

/*
 * A ContextFrameHandle stores a reference to a ContextFrame, and keeps it on
 * the special variable stack for the lifetime of the handle.
 */
class ContextFrameHandle
{
public:
  ContextFrameHandle(ContextFrame *frame);
  ~ContextFrameHandle() { release(); }

  ContextFrameHandle(const ContextFrameHandle&) = delete;
  ContextFrameHandle& operator=(const ContextFrameHandle&) = delete;
  ContextFrameHandle& operator=(ContextFrameHandle&&) = delete;

  ContextFrameHandle(ContextFrameHandle&& other) noexcept
    : session(other.session), frame_index(other.frame_index)
  {
    other.session = nullptr;
  }

  ContextFrameHandle& operator=(ContextFrame *frame);

  // Valid only if handle is on the top of the stack.
  void release();

protected:
  EvaluationSession *session;
  size_t frame_index;
};

#pragma once

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "core/Arguments.h"
#include "core/ContextFrame.h"

/*
 * The parameters of a builtin function or module do not form a true Context;
 * it is a value map, but it doesn't have a parent context or child contexts,
 * no function literals can capture it, and it has simpler memory management.
 * But special variables passed as parameters ARE accessible on the execution
 * stack. Thus, a Parameters is a ContextFrame, held by a ContextFrameHandle.
 */
class Parameters
{
private:
  Parameters(ContextFrame&& frame, Location loc);

public:
  Parameters(Parameters&& other) noexcept;

  /*
   * Matches arguments with parameters.
   * Does not support default arguments.
   * Required parameters are set to undefined if absent;
   * Optional parameters are not set at all.
   */
  static Parameters parse(
    Arguments arguments,
    const Location& loc,
    const std::vector<std::string>& required_parameters,
    const std::vector<std::string>& optional_parameters = {}
    );
  /*
   * Matches arguments with parameters.
   * Supports default arguments, and requires a context in which to interpret them.
   * Absent parameters without defaults are set to undefined.
   */
  static Parameters parse(
    Arguments arguments,
    const Location& loc,
    const AssignmentList& required_parameters,
    const std::shared_ptr<const Context>& defining_context
    );

  boost::optional<const Value&> lookup(const std::string& name) const;

  void set_caller(const std::string& caller);
  const Value& get(const std::string& name) const;
  double get(const std::string& name, double default_value) const;
  const std::string& get(const std::string& name, const std::string& default_value) const;

  bool contains(const std::string& name) const { return bool(lookup(name)); }
  const Value& operator[](const std::string& name) const { return get(name); }
  bool valid(const std::string& name, Value::Type type);
  bool valid_required(const std::string& name, Value::Type type);
  bool validate_number(const std::string& name, double& out);
  template <typename T> bool validate_integral(const std::string& name, T& out,
                                               T lo = std::numeric_limits<T>::min(),
                                               T hi = std::numeric_limits<T>::max());

  ContextFrame to_context_frame() &&;

  const std::string& documentRoot() const { return frame.documentRoot(); }
  const Location& location() const { return loc; }

private:
  Location loc;
  ContextFrame frame;
  ContextFrameHandle handle;
  bool valid(const std::string& name, const Value& value, Value::Type type);
  std::string caller = "";
};

// Silently clamp to the given range(defaults to numeric_limits)
// as long as param is a finite number.
template <typename T>
bool Parameters::validate_integral(const std::string& name, T& out, T lo, T hi)
{
  double temp;
  if (validate_number(name, temp)) {
    if (temp < lo) {
      out = lo;
    } else if (temp > hi) {
      out = hi;
    } else {
      out = static_cast<T>(temp);
    }
    return true;
  }
  return false;
}

void print_argCnt_warning(const std::string& name, int found,
                          const std::string& expected, const Location& loc,
                          const std::string& documentRoot);
void print_argConvert_positioned_warning(const std::string& calledName, const std::string& where,
                                         const Value& found, std::vector<Value::Type> expected,
                                         const Location& loc, const std::string& documentRoot);
void print_argConvert_warning(const std::string& calledName, const std::string& argName,
                              const Value& found, std::vector<Value::Type> expected,
                              const Location& loc, const std::string& documentRoot);

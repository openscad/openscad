#pragma once

#include "core/Arguments.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace FunctionArgs {

/**
 * @brief Error callback used by argument normalization.
 *
 * Callers provide builtin-specific behavior (typically logging + throw).
 */
using ErrorFn = std::function<void(const std::string&)>;

/**
 * @brief Canonical fixed-parameter declaration.
 *
 * Each entry declares one fixed parameter name and an optional default value.
 * If no default is provided and the parameter is omitted, normalization
 * produces `&Value::undefined` for that slot.
 */
struct ParamDef {
  const char *name;
  std::shared_ptr<const Value> default_value;

  ParamDef(const char *param_name) : name(param_name) {}
  ParamDef(const char *param_name, const char *default_string)
    : name(param_name), default_value(std::make_shared<Value>(default_string))
  {
  }
  ParamDef(const char *param_name, bool default_bool)
    : name(param_name), default_value(std::make_shared<Value>(default_bool))
  {
  }
  ParamDef(const char *param_name, int default_number)
    : name(param_name), default_value(std::make_shared<Value>(default_number))
  {
  }
  ParamDef(const char *param_name, double default_number)
    : name(param_name), default_value(std::make_shared<Value>(default_number))
  {
  }
  ParamDef(const char *param_name, Value default_complex)
    : name(param_name), default_value(std::make_shared<Value>(std::move(default_complex)))
  {
  }
};

/**
 * @brief Argument-shape specification for one builtin/function.
 *
 * Responsibilities:
 * - maps positional and named arguments into fixed canonical slots
 * - enforces structure rules:
 *   - duplicate named key is an error
 *   - positional after first named is an error
 * - ignores unknown named keys prefixed with '$'
 * - applies fixed-slot defaults
 * - optionally supports one named variadic block (e.g. `args=...`)
 *
 * Variadic block behavior (when `variadic_block_name` is set):
 * - named `<variadic_block_name>=...` appends its value to `variadic`
 * - positional values immediately following it are appended to `variadic`
 *   until the next named argument appears
 * - after another named argument, normal positional-after-named rules apply
 *
 * Fixed-only example (`timer_stop`):
 * @code
 * static const FunctionArgs::Spec stop_spec{
 *   "timer_stop",
 *   {
 *     {"timer_id"},
 *     {"fmt_str", "timer {n} {mmm}:{ss}.{ddd}"},
 *     {"output", false},
 *     {"delete", false},
 *   },
 * };
 *
 * const auto fixed = stop_spec.normalize(arguments, fail);
 * // fixed[0]=timer_id, fixed[1]=fmt_str, fixed[2]=output, fixed[3]=delete
 * @endcode
 *
 * Fixed + variadic block example (`timer_run`):
 * @code
 * static const FunctionArgs::Spec run_spec{
 *   "timer_run",
 *   {{"name"}, {"fn"}},
 *   "args",
 * };
 *
 * const auto normalized = run_spec.normalizeWithVariadic(arguments, fail);
 * // normalized.fixed[0]=name, normalized.fixed[1]=fn
 * // normalized.variadic is std::vector<const Value*> for args...
 * // use normalized.variadic.size() and normalized.variadic[i]
 * @endcode
 */
class Spec
{
public:
  /**
   * @brief Normalization result for fixed slots + optional variadic tail.
   */
  struct NormalizeResult {
    /// Canonical fixed slots in declaration order (`params`).
    std::vector<const Value *> fixed;
    /// Values collected into variadic block, if configured/used.
    std::vector<const Value *> variadic;
  };

  /**
   * @param function_name Function name for diagnostics.
   * @param params Fixed canonical parameters in order.
   * @param variadic_block_name Optional named variadic block key (e.g. `"args"`).
   */
  Spec(const char *function_name, std::initializer_list<ParamDef> params,
       const char *variadic_block_name = nullptr);

  /**
   * @brief Normalize fixed parameters only.
   *
   * Equivalent to `normalizeWithVariadic(...).fixed`.
   */
  std::vector<const Value *> normalize(const Arguments& arguments, const ErrorFn& fail) const;

  /**
   * @brief Normalize fixed parameters plus optional variadic block.
   */
  NormalizeResult normalizeWithVariadic(const Arguments& arguments, const ErrorFn& fail) const;

private:
  struct SlotSource {
    size_t arg_index;
    const std::string *named_key;
  };

  const char *function_name_;
  std::vector<ParamDef> params_;
  std::unordered_map<std::string, size_t> param_index_by_name_;
  const char *variadic_block_name_;
};

} // namespace FunctionArgs

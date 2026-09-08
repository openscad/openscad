#pragma once

#include "core/Arguments.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace FunctionArgs {

/**
 * @brief Error callback used by argument normalization.
 *
 * Callers provide builtin-specific behavior (typically logging + throw).
 */
using ErrorFn = std::function<void(const std::string&)>;

struct SpecVariadic {
  std::vector<std::shared_ptr<const Value>> default_values;

  SpecVariadic() = default;
  explicit SpecVariadic(const char *default_string)
  {
    default_values.emplace_back(std::make_shared<Value>(default_string));
  }
  explicit SpecVariadic(bool default_bool)
  {
    default_values.emplace_back(std::make_shared<Value>(default_bool));
  }
  explicit SpecVariadic(int default_number)
  {
    default_values.emplace_back(std::make_shared<Value>(default_number));
  }
  explicit SpecVariadic(double default_number)
  {
    default_values.emplace_back(std::make_shared<Value>(default_number));
  }
  explicit SpecVariadic(Value default_complex)
  {
    default_values.emplace_back(std::make_shared<Value>(std::move(default_complex)));
  }
};

struct FixedParamDef {
  const char *name;
  std::shared_ptr<const Value> default_value;
};

struct VariadicParamDef {
  const char *name;
  std::vector<std::shared_ptr<const Value>> default_values;
};

/**
 * @brief Canonical parameter declaration.
 *
 * Each entry declares one parameter name and either:
 * - one fixed slot with optional default value
 * - one variadic slot with optional default values
 */
struct ParamDef {
  std::variant<FixedParamDef, VariadicParamDef> param;

  ParamDef(const char *param_name) : param(FixedParamDef{param_name, nullptr}) {}
  ParamDef(const char *param_name, const char *default_string)
    : param(FixedParamDef{param_name, std::make_shared<Value>(default_string)})
  {
  }
  ParamDef(const char *param_name, bool default_bool)
    : param(FixedParamDef{param_name, std::make_shared<Value>(default_bool)})
  {
  }
  ParamDef(const char *param_name, int default_number)
    : param(FixedParamDef{param_name, std::make_shared<Value>(default_number)})
  {
  }
  ParamDef(const char *param_name, double default_number)
    : param(FixedParamDef{param_name, std::make_shared<Value>(default_number)})
  {
  }
  ParamDef(const char *param_name, Value default_complex)
    : param(FixedParamDef{param_name, std::make_shared<Value>(std::move(default_complex))})
  {
  }
  ParamDef(const char *param_name, SpecVariadic variadic_defaults)
    : param(VariadicParamDef{param_name, std::move(variadic_defaults.default_values)})
  {
  }

  const char *name() const
  {
    return std::visit([](const auto& p) { return p.name; }, param);
  }

  bool isVariadic() const { return std::holds_alternative<VariadicParamDef>(param); }

  const std::shared_ptr<const Value>& defaultValue() const
  {
    static const std::shared_ptr<const Value> empty;
    if (const auto *fixed = std::get_if<FixedParamDef>(&param)) {
      return fixed->default_value;
    }
    return empty;
  }

  const std::vector<std::shared_ptr<const Value>>& variadicDefaultValues() const
  {
    static const std::vector<std::shared_ptr<const Value>> empty;
    if (const auto *variadic = std::get_if<VariadicParamDef>(&param)) {
      return variadic->default_values;
    }
    return empty;
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
 * - optionally supports variadic slots in `params`
 *
 * Variadic slot behavior:
 * - named `<variadic_name>=...` appends its value to that variadic slot
 * - positional values immediately following it are appended to `variadic`
 *   until the next named argument appears
 * - parameters declared after the first variadic must be passed by name
 * - after another named argument, normal positional-after-named rules apply
 *
 * Fixed-only example (`timer_stop`):
 * @code
 * static const FunctionArgs::Spec stop_spec{
 *   "timer_stop",
 *   {
 *     {"timer_id"},
 *     {"fmt_str", "timer {n} {mmm}:{ss}.{ddd}"},
 *     {"iterations", 1},
 *     {"output", false},
 *     {"delete", false},
 *   },
 * };
 *
 * const auto normalized = stop_spec.normalize(arguments, fail);
 * // normalized.params[0]=timer_id, normalized.params[1]=fmt_str
 * // normalized.params[2]=iterations, normalized.params[3]=output, normalized.params[4]=delete
 * @endcode
 *
 * Fixed + variadic block example (`timer_run`):
 * @code
 * static const FunctionArgs::Spec run_spec{
 *   "timer_run",
 *   {{"name"}, {"fn"}, {"args", Spec::Variadic{}}},
 * };
 *
 * const auto normalized = run_spec.normalize(arguments, fail);
 * // normalized.params[0]=name, normalized.params[1]=fn
 * // normalized.params[2] is list Value for args...
 * @endcode
 */
class Spec
{
public:
  using Variadic = SpecVariadic;

  /**
   * @brief Normalization result in canonical `params` order.
   */
  struct NormalizeResult {
    /// Canonical values in declaration order (`params`).
    std::vector<const Value *> params;
    /// Owns synthesized values (for example variadic list materialization).
    std::vector<std::shared_ptr<const Value>> owned_values;

    const Value *operator[](size_t index) const { return params[index]; }
    size_t size() const { return params.size(); }
  };

  /**
   * @param function_name Function name for diagnostics.
   * @param params Canonical parameters in order.
   */
  Spec(const char *function_name, std::initializer_list<ParamDef> params);

  NormalizeResult normalize(const Arguments& arguments, const ErrorFn& fail) const;

private:
  struct SlotSource {
    size_t arg_index;
    const std::string *named_key;
  };

  const char *function_name_;
  std::vector<ParamDef> params_;
  std::unordered_map<std::string, size_t> param_index_by_name_;
};

}  // namespace FunctionArgs

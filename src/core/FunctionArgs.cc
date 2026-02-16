#include "core/FunctionArgs.h"

#include "utils/printutils.h"

#include <optional>
#include <stdexcept>
namespace FunctionArgs {

Spec::Spec(const char *function_name, std::initializer_list<ParamDef> params)
  : function_name_(function_name), params_(params)
{
  size_t variadic_count = 0;
  for (size_t i = 0; i < params_.size(); ++i) {
    const auto [it, inserted] = param_index_by_name_.emplace(params_[i].name, i);
    if (!inserted) {
      throw std::logic_error(STR("Duplicate parameter name in FunctionArgs::Spec: ", params_[i].name));
    }
    if (params_[i].is_variadic) {
      variadic_count++;
      if (variadic_count > 1) {
        throw std::logic_error(STR("FunctionArgs::Spec supports at most one variadic parameter: ", function_name_));
      }
    }
  }
}

std::vector<const Value *> Spec::normalize(const Arguments& arguments, const ErrorFn& fail) const
{
  return normalizeWithVariadic(arguments, fail).fixed;
}

Spec::NormalizeResult Spec::normalizeWithVariadic(const Arguments& arguments, const ErrorFn& fail) const
{
  NormalizeResult result;
  std::vector<size_t> fixed_index_by_param(params_.size(), params_.size());
  size_t fixed_count = 0;
  size_t variadic_param_index = params_.size();
  for (size_t i = 0; i < params_.size(); ++i) {
    if (params_[i].is_variadic) {
      variadic_param_index = i;
    } else {
      fixed_index_by_param[i] = fixed_count++;
    }
  }

  result.fixed.assign(fixed_count, &Value::undefined);
  std::vector<std::optional<SlotSource>> slot_sources(fixed_count);

  std::unordered_map<std::string, size_t> named_first_arg;
  bool seen_named = false;
  bool in_variadic_block = false;
  bool variadic_assigned = false;
  const std::string *first_named_key = nullptr;
  size_t first_named_arg_index = 0;
  size_t positional_index = 0;

  const auto source_desc = [&](const SlotSource& source) {
    if (source.named_key) {
      return STR("named argument '", *source.named_key, "' at argument ", source.arg_index);
    }
    return STR("positional argument ", source.arg_index);
  };

  const auto assign = [&](size_t param_index, const Value *value, bool from_named, size_t arg_index,
                          const std::string *named_key) {
    if (param_index >= params_.size() || params_[param_index].is_variadic) {
      fail(STR(function_name_, "() internal error: parameter index out of bounds"));
    }
    const size_t fixed_index = fixed_index_by_param[param_index];
    if (fixed_index >= slot_sources.size()) {
      fail(STR(function_name_, "() internal error: fixed parameter index out of bounds"));
    }
    if (slot_sources[fixed_index]) {
      const auto& previous = *slot_sources[fixed_index];
      const std::string current_desc =
        from_named ? STR("named argument '", *named_key, "' at argument ", arg_index)
                   : STR("positional argument ", arg_index);
      fail(STR(function_name_, "() parameter '", params_[param_index].name, "' was already set by ",
               source_desc(previous), "; cannot set again by ", current_desc));
    }
    slot_sources[fixed_index] = SlotSource{arg_index, from_named ? named_key : nullptr};
    result.fixed[fixed_index] = value;
  };

  for (size_t i = 0; i < arguments.size(); ++i) {
    const auto& argument = arguments[i];
    const size_t arg_index = i + 1;

    if (argument.name) {
      const std::string& key = *argument.name;

      if (named_first_arg.count(key)) {
        fail(STR(function_name_, "() named argument '", key, "' supplied more than once (at arguments ",
                 named_first_arg[key], " and ", arg_index, ")"));
      }
      named_first_arg.emplace(key, arg_index);

      if (!seen_named) {
        seen_named = true;
        first_named_key = &key;
        first_named_arg_index = arg_index;
      }

      const auto it = param_index_by_name_.find(key);
      if (it == param_index_by_name_.end()) {
        if (!key.empty() && key[0] == '$') {
          continue;
        }
        fail(STR(function_name_, "() unknown named argument '", key, "'"));
      }

      const size_t param_index = it->second;
      if (params_[param_index].is_variadic) {
        in_variadic_block = true;
        variadic_assigned = true;
        result.variadic.emplace_back(&argument.value);
        continue;
      }

      in_variadic_block = false;
      assign(param_index, &argument.value, true, arg_index, &key);
    } else {
      if (seen_named && !in_variadic_block) {
        fail(STR(function_name_, "() positional argument ", arg_index,
                 " is not allowed after named argument '", *first_named_key, "' at argument ",
                 first_named_arg_index));
      }

      if (seen_named && in_variadic_block) {
        variadic_assigned = true;
        result.variadic.emplace_back(&argument.value);
        continue;
      }

      if (variadic_param_index < params_.size() && positional_index >= variadic_param_index) {
        variadic_assigned = true;
        result.variadic.emplace_back(&argument.value);
        continue;
      }

      const size_t param_index = positional_index++;
      if (param_index >= params_.size()) {
        if (variadic_param_index < params_.size()) {
          variadic_assigned = true;
          result.variadic.emplace_back(&argument.value);
          continue;
        }
        fail(STR(function_name_, "() expected up to ", params_.size(), " positional arguments, got ",
                 positional_index));
      }
      assign(param_index, &argument.value, false, arg_index, nullptr);
    }
  }

  for (size_t i = 0; i < params_.size(); ++i) {
    if (params_[i].is_variadic) {
      continue;
    }
    const size_t fixed_index = fixed_index_by_param[i];
    if (fixed_index >= slot_sources.size()) {
      fail(STR(function_name_, "() internal error: fixed parameter index out of bounds"));
    }
    if (slot_sources[fixed_index]) {
      continue;
    }
    if (params_[i].default_value) {
      result.fixed[fixed_index] = params_[i].default_value.get();
    } else {
      result.fixed[fixed_index] = &Value::undefined;
    }
  }

  if (!variadic_assigned && variadic_param_index < params_.size()) {
    for (const auto& value : params_[variadic_param_index].variadic_default_values) {
      result.variadic.emplace_back(value.get());
    }
  }

  return result;
}

}  // namespace FunctionArgs

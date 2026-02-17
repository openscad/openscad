#include "core/FunctionArgs.h"

#include "utils/printutils.h"

#include <optional>
#include <stdexcept>
namespace FunctionArgs {

Spec::Spec(const char *function_name, std::initializer_list<ParamDef> params)
  : function_name_(function_name), params_(params)
{
  for (size_t i = 0; i < params_.size(); ++i) {
    const auto [it, inserted] = param_index_by_name_.emplace(params_[i].name(), i);
    if (!inserted) {
      throw std::logic_error(STR("Duplicate parameter name in FunctionArgs::Spec: ", params_[i].name()));
    }
  }
}

Spec::NormalizeResult Spec::normalize(const Arguments& arguments, const ErrorFn& fail) const
{
  NormalizeResult result;
  size_t first_variadic_param_index = params_.size();
  for (size_t i = 0; i < params_.size(); ++i) {
    if (params_[i].isVariadic()) {
      if (first_variadic_param_index >= params_.size()) {
        first_variadic_param_index = i;
      }
    }
  }

  result.params.assign(params_.size(), &Value::undefined);
  std::vector<std::optional<SlotSource>> slot_sources(params_.size());
  std::vector<bool> variadic_assigned(params_.size(), false);
  std::vector<std::vector<const Value *>> variadic_values_by_param(params_.size());

  std::unordered_map<std::string, size_t> named_first_arg;
  bool seen_named = false;
  size_t active_variadic_param_index = params_.size();
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
    if (param_index >= params_.size() || params_[param_index].isVariadic()) {
      fail(STR(function_name_, "() internal error: parameter index out of bounds"));
    }
    if (param_index >= slot_sources.size()) {
      fail(STR(function_name_, "() internal error: fixed parameter index out of bounds"));
    }
    if (slot_sources[param_index]) {
      const auto& previous = *slot_sources[param_index];
      const std::string current_desc =
        from_named ? STR("named argument '", *named_key, "' at argument ", arg_index)
                   : STR("positional argument ", arg_index);
      fail(STR(function_name_, "() parameter '", params_[param_index].name(), "' was already set by ",
               source_desc(previous), "; cannot set again by ", current_desc));
    }
    slot_sources[param_index] = SlotSource{arg_index, from_named ? named_key : nullptr};
    result.params[param_index] = value;
  };

  const auto append_variadic = [&](size_t param_index, const Value *value) {
    if (param_index >= params_.size() || !params_[param_index].isVariadic()) {
      fail(STR(function_name_, "() internal error: variadic parameter index out of bounds"));
    }
    variadic_assigned[param_index] = true;
    variadic_values_by_param[param_index].emplace_back(value);
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
      if (params_[param_index].isVariadic()) {
        active_variadic_param_index = param_index;
        append_variadic(param_index, &argument.value);
        continue;
      }

      active_variadic_param_index = params_.size();
      assign(param_index, &argument.value, true, arg_index, &key);
    } else {
      if (seen_named && active_variadic_param_index >= params_.size()) {
        fail(STR(function_name_, "() positional argument ", arg_index,
                 " is not allowed after named argument '", *first_named_key, "' at argument ",
                 first_named_arg_index));
      }

      if (seen_named && active_variadic_param_index < params_.size()) {
        append_variadic(active_variadic_param_index, &argument.value);
        continue;
      }

      if (first_variadic_param_index < params_.size() && positional_index >= first_variadic_param_index) {
        append_variadic(first_variadic_param_index, &argument.value);
        continue;
      }

      const size_t param_index = positional_index++;
      if (param_index >= params_.size()) {
        if (first_variadic_param_index < params_.size()) {
          append_variadic(first_variadic_param_index, &argument.value);
          continue;
        }
        fail(STR(function_name_, "() expected up to ", params_.size(), " positional arguments, got ",
                 positional_index));
      }
      assign(param_index, &argument.value, false, arg_index, nullptr);
    }
  }

  for (size_t i = 0; i < params_.size(); ++i) {
    if (params_[i].isVariadic()) {
      continue;
    }
    if (i >= slot_sources.size()) {
      fail(STR(function_name_, "() internal error: fixed parameter index out of bounds"));
    }
    if (slot_sources[i]) {
      continue;
    }
    if (params_[i].defaultValue()) {
      result.params[i] = params_[i].defaultValue().get();
    } else {
      result.params[i] = &Value::undefined;
    }
  }

  for (size_t i = 0; i < params_.size(); ++i) {
    if (!params_[i].isVariadic() || variadic_assigned[i]) {
      continue;
    }
    for (const auto& value : params_[i].variadicDefaultValues()) {
      variadic_values_by_param[i].emplace_back(value.get());
    }
  }

  for (size_t i = 0; i < params_.size(); ++i) {
    if (!params_[i].isVariadic()) {
      continue;
    }
    VectorType vec(arguments.session());
    vec.reserve(variadic_values_by_param[i].size());
    for (const Value *value : variadic_values_by_param[i]) {
      vec.emplace_back(value->clone());
    }
    auto list_value = std::make_shared<Value>(std::move(vec));
    result.params[i] = list_value.get();
    result.owned_values.emplace_back(std::move(list_value));
  }

  return result;
}

}  // namespace FunctionArgs

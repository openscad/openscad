#include "core/FunctionArgs.h"

#include "utils/printutils.h"

#include <optional>
#include <stdexcept>
namespace FunctionArgs {

Spec::Spec(const char *function_name, std::initializer_list<ParamDef> params,
           const char *variadic_block_name)
  : function_name_(function_name), params_(params), variadic_block_name_(variadic_block_name)
{
  for (size_t i = 0; i < params_.size(); ++i) {
    const auto [it, inserted] = param_index_by_name_.emplace(params_[i].name, i);
    if (!inserted) {
      throw std::logic_error(STR("Duplicate parameter name in FunctionArgs::Spec: ", params_[i].name));
    }
  }
}

std::vector<const Value *> Spec::normalize(const Arguments& arguments, const ErrorFn& fail) const
{
  return normalizeWithVariadic(arguments, fail).fixed;
}

Spec::NormalizeResult Spec::normalizeWithVariadic(const Arguments& arguments,
                                                  const ErrorFn& fail) const
{
  NormalizeResult result;
  result.fixed.assign(params_.size(), &Value::undefined);
  std::vector<std::optional<SlotSource>> slot_sources(params_.size());

  std::unordered_map<std::string, size_t> named_first_arg;
  bool seen_named = false;
  bool in_variadic_block = false;
  const std::string *first_named_key = nullptr;
  size_t first_named_arg_index = 0;
  size_t positional_index = 0;

  const auto source_desc = [&](const SlotSource& source) {
    if (source.named_key) {
      return STR("named argument '", *source.named_key, "' at argument ", source.arg_index);
    }
    return STR("positional argument ", source.arg_index);
  };

  const auto assign = [&](size_t param_index, const Value *value, bool from_named,
                          size_t arg_index, const std::string *named_key) {
    if (param_index >= params_.size()) {
      fail(STR(function_name_, "() internal error: parameter index out of bounds"));
    }
    if (slot_sources[param_index]) {
      const auto& previous = *slot_sources[param_index];
      const std::string current_desc = from_named
                                         ? STR("named argument '", *named_key, "' at argument ", arg_index)
                                         : STR("positional argument ", arg_index);
      fail(STR(function_name_, "() parameter '", params_[param_index].name,
               "' was already set by ", source_desc(previous),
               "; cannot set again by ", current_desc));
    }
    slot_sources[param_index] = SlotSource{arg_index, from_named ? named_key : nullptr};
    result.fixed[param_index] = value;
  };

  for (size_t i = 0; i < arguments.size(); ++i) {
    const auto& argument = arguments[i];
    const size_t arg_index = i + 1;

    if (argument.name) {
      const std::string& key = *argument.name;

      if (named_first_arg.count(key)) {
        fail(STR(function_name_, "() named argument '", key,
                 "' supplied more than once (at arguments ", named_first_arg[key],
                 " and ", arg_index, ")"));
      }
      named_first_arg.emplace(key, arg_index);

      if (!seen_named) {
        seen_named = true;
        first_named_key = &key;
        first_named_arg_index = arg_index;
      }

      const bool is_variadic_block_name =
        variadic_block_name_ && key == variadic_block_name_;
      if (is_variadic_block_name) {
        in_variadic_block = true;
        result.variadic.emplace_back(&argument.value);
        continue;
      }

      in_variadic_block = false;
      const auto it = param_index_by_name_.find(key);
      if (it == param_index_by_name_.end()) {
        if (!key.empty() && key[0] == '$') {
          continue;
        }
        fail(STR(function_name_, "() unknown named argument '", key, "'"));
      }

      assign(it->second, &argument.value, true, arg_index, &key);
    } else {
      if (seen_named && !in_variadic_block) {
        fail(STR(function_name_, "() positional argument ", arg_index,
                 " is not allowed after named argument '", *first_named_key,
                 "' at argument ", first_named_arg_index));
      }

      if (seen_named && in_variadic_block) {
        result.variadic.emplace_back(&argument.value);
        continue;
      }

      const size_t param_index = positional_index++;
      if (param_index >= params_.size()) {
        if (variadic_block_name_) {
          result.variadic.emplace_back(&argument.value);
          continue;
        }
        fail(STR(function_name_, "() expected up to ", params_.size(),
                 " positional arguments, got ", positional_index));
      }
      assign(param_index, &argument.value, false, arg_index, nullptr);
    }
  }

  for (size_t i = 0; i < params_.size(); ++i) {
    if (slot_sources[i]) {
      continue;
    }
    if (params_[i].default_value) {
      result.fixed[i] = params_[i].default_value.get();
    } else {
      result.fixed[i] = &Value::undefined;
    }
  }

  return result;
}

} // namespace FunctionArgs

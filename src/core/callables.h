#pragma once

#include <memory>
#include <variant>

#include "core/Value.h"

class Context;
class UserFunction;
class BuiltinFunction;
class AbstractModule;

struct CallableUserFunction {
  std::shared_ptr<const Context> defining_context;
  const UserFunction *function;
};
using CallableFunction =
  std::variant<const BuiltinFunction *, CallableUserFunction, Value, const Value *>;

struct InstantiableModule {
  std::shared_ptr<const Context> defining_context;
  const AbstractModule *module;
};

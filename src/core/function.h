#pragma once

#include "core/AST.h"
#include "core/Assignment.h"
#include "Feature.h"
#include "core/Value.h"

#include <ostream>
#include <memory>
#include <functional>
#include <string>
#include <variant>

class Arguments;
class FunctionCall;

class BuiltinFunction
{
public:
  typedef std::function<Value(const std::shared_ptr<const Context>&, const FunctionCall *)> evaluate_t;
  evaluate_t evaluate;

private:
  const Feature *feature;

public:
  BuiltinFunction(const evaluate_t& f, const Feature *feature = nullptr);
  BuiltinFunction(Value(*f)(Arguments, const Location&), const Feature *feature = nullptr);

  [[nodiscard]] bool is_experimental() const { return feature != nullptr; }
  [[nodiscard]] bool is_enabled() const { return (feature == nullptr) || feature->is_enabled(); }
};

class UserFunction : public ASTNode
{
public:
  std::string name;
  AssignmentList parameters;
  std::shared_ptr<Expression> expr;

  UserFunction(const char *name, AssignmentList& parameters, std::shared_ptr<Expression> expr, const Location& loc);

  void print(std::ostream& stream, const std::string& indent) const override;
};


struct CallableUserFunction
{
  std::shared_ptr<const Context> defining_context;
  const UserFunction *function;
};

using CallableFunction = std::variant<
  const BuiltinFunction *,
  CallableUserFunction,
  Value,
  const Value *,
  std::shared_ptr<const BuiltinFunction>
>;

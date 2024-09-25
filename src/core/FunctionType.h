#pragma once

#include <utility>
#include <memory>
#include <ostream>

#include "core/Assignment.h"

class Context;
class Expression;
class Value;

class FunctionType
{
public:
  FunctionType(std::shared_ptr<const Context> context, std::shared_ptr<Expression> expr, std::shared_ptr<AssignmentList> parameters)
    : context(std::move(context)), expr(std::move(expr)), parameters(std::move(parameters)) { }
  Value operator==(const FunctionType& other) const;
  Value operator!=(const FunctionType& other) const;
  Value operator<(const FunctionType& other) const;
  Value operator>(const FunctionType& other) const;
  Value operator<=(const FunctionType& other) const;
  Value operator>=(const FunctionType& other) const;

  [[nodiscard]] const std::shared_ptr<const Context>& getContext() const { return context; }
  [[nodiscard]] const std::shared_ptr<Expression>& getExpr() const { return expr; }
  [[nodiscard]] const std::shared_ptr<AssignmentList>& getParameters() const { return parameters; }
private:
  std::shared_ptr<const Context> context;
  std::shared_ptr<Expression> expr;
  std::shared_ptr<AssignmentList> parameters;
};

std::ostream& operator<<(std::ostream& stream, const FunctionType& f);

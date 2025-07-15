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
  
  /**
    Set the receiver of this function, making it a method. The value
    v must be an object but is passed as Value because of the circular 
    dependency between FunctionType and Value.
  */
  void set_receiver(Value v);
  std::shared_ptr<Value> get_receiver() const;
  std::vector<Value> * get_values() const;
private:
  std::shared_ptr<const Context> context;
  std::shared_ptr<Expression> expr;
  std::shared_ptr<AssignmentList> parameters;
  std::shared_ptr<std::vector<Value>> receiver{std::make_shared<std::vector<Value>>()};
};

std::ostream& operator<<(std::ostream& stream, const FunctionType& f);

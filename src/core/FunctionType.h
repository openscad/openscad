#pragma once

#include <utility>
#include <memory>
#include <ostream>

#include "core/Assignment.h"

class Context;
class Expression;
class Value;
class ObjectObject;
class FunctionType
{
public:
  FunctionType(std::shared_ptr<const Context> context, std::shared_ptr<Expression> expr, std::shared_ptr<AssignmentList> parameters)
    : context(std::move(context)), expr(std::move(expr)), parameters(std::move(parameters)) { }

  FunctionType(const FunctionType& from,
               const std::shared_ptr<ObjectObject>& receiver)
    : FunctionType(from.context, from.expr, from.parameters)
  {
    this->receiver = receiver;
  }

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
  const std::weak_ptr<ObjectObject> & get_receiver() const;
private:
  std::shared_ptr<const Context> context;
  std::shared_ptr<Expression> expr;
  std::shared_ptr<AssignmentList> parameters;
  std::weak_ptr<ObjectObject> receiver;
};

std::ostream& operator<<(std::ostream& stream, const FunctionType& f);

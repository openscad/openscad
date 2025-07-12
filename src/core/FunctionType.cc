#include "core/FunctionType.h"

#include <ostream>
#include "core/Value.h"
#include "core/Expression.h"

Value FunctionType::operator==(const FunctionType& other) const {
  return this == &other;
}
Value FunctionType::operator!=(const FunctionType& other) const {
  return this != &other;
}
Value FunctionType::operator<(const FunctionType& /*other*/) const {
  return Value::undef("operation undefined (function < function)");
}
Value FunctionType::operator>(const FunctionType& /*other*/) const {
  return Value::undef("operation undefined (function > function)");
}
Value FunctionType::operator<=(const FunctionType& /*other*/) const {
  return Value::undef("operation undefined (function <= function)");
}
Value FunctionType::operator>=(const FunctionType& /*other*/) const {
  return Value::undef("operation undefined (function >= function)");
}

std::shared_ptr<Value> FunctionType::get_receiver() const {
    if ( receiver->empty() ) {
        std::shared_ptr<Value> empty;
        return std::move(empty);
    } else {
        auto value = receiver->at(0).clone();
        return std::make_shared<Value>(std::move(value));
    }
}

void FunctionType::set_receiver(Value value) {
    receiver = std::make_shared<std::vector<Value>>();
    receiver->emplace_back(std::move(value));
}

std::ostream& operator<<(std::ostream& stream, const FunctionType& f)
{
  stream << "function(";
  bool first = true;
  for (const auto& parameter : *(f.getParameters())) {
    stream << (first ? "" : ", ") << parameter->getName();
    if (parameter->getExpr()) {
      stream << " = " << *parameter->getExpr();
    }
    first = false;
  }
  stream << ") " << *f.getExpr();
  return stream;
}

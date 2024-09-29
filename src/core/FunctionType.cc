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

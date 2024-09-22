#include "core/Value.h"

Value UndefType::operator<(const UndefType& /*other*/) const {
  return Value::undef("operation undefined (undefined < undefined)");
}
Value UndefType::operator>(const UndefType& /*other*/) const {
  return Value::undef("operation undefined (undefined > undefined)");
}
Value UndefType::operator<=(const UndefType& /*other*/) const {
  return Value::undef("operation undefined (undefined <= undefined)");
}
Value UndefType::operator>=(const UndefType& /*other*/) const {
  return Value::undef("operation undefined (undefined >= undefined)");
}

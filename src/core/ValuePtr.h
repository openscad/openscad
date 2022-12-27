#pragma once

#include <memory>

template <typename T>
class ValuePtr
{
private:
  explicit ValuePtr(std::shared_ptr<T> val_in) : value(std::move(val_in)) { }
public:
  ValuePtr(T&& value) : value(std::make_shared<T>(std::move(value))) { }
  [[nodiscard]] ValuePtr clone() const { return ValuePtr(value); }

  const T& operator*() const { return *value; }
  const T *operator->() const { return value.get(); }
  [[nodiscard]] const std::shared_ptr<T>& get() const { return value; }

private:
  std::shared_ptr<T> value;
};




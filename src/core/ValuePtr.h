#pragma once

#include <memory>

template <typename T>
class ValuePtr
{
private:
  explicit ValuePtr(const std::shared_ptr<T>& val_in) : value(val_in) { }
public:
  ValuePtr(T&& value) : value(std::make_shared<T>(std::move(value))) { }
  ValuePtr clone() const { return ValuePtr(value); }

  const T& operator*() const { return *value; }
  const T *operator->() const { return value.get(); }
  const std::shared_ptr<T>& get() const { return value; }

private:
  std::shared_ptr<T> value;
};


#pragma once

#include <iterator>
#include <limits>
#include <cstdint>
#include <ostream>
#include <cmath>

class PythonClassType
{
public:
  PythonClassType(const PythonClassType&) = delete;             // never copy, move instead
  PythonClassType& operator=(const PythonClassType&) = delete;  // never copy, move instead
  PythonClassType(PythonClassType&&) = default;
  PythonClassType& operator=(PythonClassType&&) = default;
  ~PythonClassType() = default;

  explicit PythonClassType(void *python_ptr) : ptr(python_ptr) {}

  bool operator==(const PythonClassType& other) const { return other.ptr == ptr; }

  bool operator!=(const PythonClassType& other) const { return !(*this == other); }

  bool operator<(const PythonClassType& other) const { return (ptr < other.ptr); }

  bool operator<=(const PythonClassType& other) const { return (ptr <= other.ptr); }

  bool operator>(const PythonClassType& other) const { return (ptr > other.ptr); }

  bool operator>=(const PythonClassType& other) const { return (ptr >= other.ptr); }
  void *ptr;
};

std::ostream& operator<<(std::ostream& stream, const PythonClassType& r);

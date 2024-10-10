#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "utils/hash.h" // IWYU pragma: keep

/*!
   Reindexes a collection of elements of type T.
   Typically used to compress an element array by creating and reusing indexes to
   a new array or to merge two index tables to two arrays into a common index.
   The latter is necessary for VBO's or for unifying texture coordinate indices to
   multiple texture coordinate arrays.
 */
template <typename T>
class Reindexer
{
public:
  /*!
     Looks up a value. Will insert the value if it doesn't already exist.
     Returns the new index. */
  int lookup(const T& val) {
    auto iter = this->map.find(val);
    if (iter != this->map.end()) return iter->second;
    else {
      this->map.insert(std::make_pair(val, this->map.size()));
      return this->map.size() - 1;
    }
  }

  /*!
     Returns the current size of the new element array
   */
  [[nodiscard]] std::size_t size() const {
    return this->map.size();
  }

  /*!
     Reserve the requested size for the new element map
   */
  void reserve(std::size_t n) {
    return this->map.reserve(n);
  }

  /*!
     Return a copy of the new element array
   */
  const std::vector<T>& getArray() {
    this->vec.resize(this->map.size());
    for (const auto& entry : map) {
      this->vec[entry.second] = entry.first;
    }
    return this->vec;
  }

  /*!
     Copies the internal vector to the given destination
   */
  template <class OutputIterator> void copy(OutputIterator dest) {
    this->getArray();
    std::copy(this->vec.begin(), this->vec.end(), dest);
  }

private:
  std::unordered_map<T, int> map;
  std::vector<T> vec;
};

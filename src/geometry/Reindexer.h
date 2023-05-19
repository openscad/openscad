#pragma once

#include <unordered_map>
#include <functional>
#include <vector>
#include <algorithm>
#include "hash.h" // IWYU pragma: keep

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
      assert(map.size() == vec.size());
      auto idx = this->map.size();
      this->map.insert(std::make_pair(val, idx));
      this->vec.push_back(val);
      return idx;
    }
  }

  /*!
     Returns the current size of the new element array
   */
  [[nodiscard]] std::size_t size() const {
    return this->vec.size();
  }

  /*!
     Reserve the requested size for the new element map
   */
  void reserve(std::size_t n) {
    this->map.reserve(n);
    this->vec.reserve(n);
  }

  /*!
     Return a reference to the element array
   */
  const std::vector<T>& getArray() {
    return this->vec;
  }

  /*!
     Copies the internal vector to the given destination
   */
  template <class OutputIterator> void copy(OutputIterator dest) {
    std::copy(this->vec.begin(), this->vec.end(), dest);
  }

private:
  std::unordered_map<T, int> map;
  std::vector<T> vec;
};

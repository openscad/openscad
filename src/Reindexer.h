#pragma once

#include <boost/unordered_map.hpp>
#include <boost/functional.hpp>
#include <vector>
#include <algorithm>

/*!
  Reindexes a collection of elements of type T.
  Typically used to compress an element array by creating and reusing indexes to 
  a new array or to merge two index tables to two arrays into a common index.
  The latter is necessary for VBO's or for unifying texture coordinate indices to
  multiple texture coordinate arrays.
*/
template<typename T>
class Reindexer
{
public:
  /*!
    Looks up a value. Will insert the value if it doesn't already exist.
    Returns the new index. */
  int lookup(const T &val) {
    typename boost::unordered_map<T, int>::const_iterator iter = this->map.find(val);
    if (iter != this->map.end()) return iter->second;
    else {
      this->map.insert(std::make_pair(val, this->map.size()));
      return this->map.size() - 1;
    }
  }

  /*!
    Returns the current size of the new element array
  */
  std::size_t size() const {
    return this->map.size();
  }

  /*!
    Return the new element array.
  */
  const T *getArray() {
    this->vec.resize(this->map.size());
    typename boost::unordered_map<T, int>::const_iterator iter = this->map.begin();
    while (iter != this->map.end()) {
      this->vec[iter->second] = iter->first;
      iter++;
    }
    return &this->vec[0];
  }

  /*!
    Copies the internal vector to the given destination
  */
  void copy(std::vector<T> &dest) {
    this->getArray();
    dest.resize(this->vec.size());
    std::copy(this->vec.begin(), this->vec.end(), dest.begin());
  }

private:
  boost::unordered_map<T, int> map;
  std::vector<T> vec;
};

#pragma once
#include "value.h"
#include <utility>
#include <unordered_map>

// Wrapper for provide *futuristic* unordered_map features,
// plus some functions specialized to our use case.
class ValueMap
{
  using map_t = std::unordered_map<std::string, Value>;
  map_t map;

public:
  using iterator = map_t::iterator;
  using const_iterator = map_t::const_iterator;

  // Workaround for function not existing in C++14
  //  - Purpose is to replace assignments such as:  map[key] = value;
  //      That would default construct a Value when insert is required,
  //      but Value() is private now.
  //  - Should not be necessary once upgraded to C++17
  //  - Doesn't bother with return type
  bool insert_or_assign(const std::string& name, Value&& value) {
    auto result = map.find(name);
    if (result == map.end()) {
      map.emplace(name, std::move(value));
      return true;
    } else {
      std::swap(result->second, value);
      return false;
    }
  }
  // Gotta have C++20 for this beast
  bool contains(const std::string& name) const { return map.count(name); }

// Directly wrapped calls
  const_iterator find(const std::string& name) const {  return map.find(name); }
  const_iterator begin() const {  return map.cbegin(); }
  const_iterator end() const {  return map.cend(); }
  iterator begin() {  return map.begin(); }
  iterator end() {  return map.end(); }
  void clear() { map.clear(); }
  size_t size() const { return map.size(); }
  template <typename ... Args> std::pair<iterator, bool> emplace(Args&&... args) {
    return map.emplace(std::forward<Args>(args)...);
  }

  // Get value by name, without possibility of default-constructing a missing name
  //   return Value::undefined if key missing
  const Value& get(const std::string& name) const {
    auto result = map.find(name);
    return result == map.end() ? Value::undefined : result->second;
  }
};

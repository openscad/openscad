#pragma once
#include "core/Value.h"

#include <cstddef>
#include <string>
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
  std::pair<iterator, bool> insert_or_assign(const std::string& name, Value&& value) {
    return map.insert_or_assign(name, std::move(value));
  }

  // Get value by name, without possibility of default-constructing a missing name
  //   return Value::undefined if key missing
  const Value& get(const std::string& name) const {
    auto result = map.find(name);
    return result == map.end() ? Value::undefined : result->second;
  }
};

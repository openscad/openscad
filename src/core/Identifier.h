#pragma once
#include <string>
#include <unordered_map>
#include <boost/optional.hpp>
#include "AST.h"

class BuiltinFunction;

class Identifier
{
public:
  Identifier(const std::string &name = "", const Location &loc = Location::NONE) : loc_(loc) {
    update(name);
  }
  Identifier(const char *name, const Location &loc = Location::NONE) : loc_(loc) {
    update(name);
  }
  
  Identifier &operator=(const std::string& v) {
    update(v);
    return *this;
  }
  bool operator== (const Identifier& other) const {
    // This is just pointer equality between interned strings.
    return interned_name_ptr_ == other.interned_name_ptr_;
  }
  bool operator<(const Identifier& other) const {
    return getName() < other.getName();
  }
  bool operator== (const char *other) const {
    return getName() == other;
  }
  bool operator== (const std::string &other) const {
    return getName() == other;
  }
  operator const std::string&() const { return getName(); }
  const std::string &getName() const { return *interned_name_ptr_; }
  size_t getHash() const { return hash_; }

  const char *c_str() const {
    return interned_name_ptr_->c_str();
  }
  bool empty() const {
    return interned_name_ptr_->empty();
  }
  const Location &location() const {
    return loc_;
  }
  bool is_config_variable() const {
    return is_config_variable_;
  }

private:

  void update(const std::string &name);

  bool is_config_variable_ = false;
  std::string *interned_name_ptr_ = NULL;
  size_t hash_ = 0;
  Location loc_;

  static std::unordered_map<std::string, std::pair<std::string, size_t>> interned_names_with_hash_;
};

inline std::ostream& operator<<(std::ostream& stream, const Identifier& id) {
  stream << id.getName();
  return stream;
}

namespace std {
  template <> struct hash<Identifier> {
    inline std::size_t operator()(const Identifier& s) const {
      return s.getHash();
    }
  };
}

#pragma once
#include <string>
#include <unordered_map>
#include <boost/optional.hpp>
#include "AST.h"

class BuiltinFunction;

class Identifier
{
public:
  Identifier() : loc(Location::NONE) {
    update("");
  }
  Identifier(const std::string &name, const Location &loc) : loc(loc) {
    update(name);
  }
  Identifier(const char *name, const Location &loc = Location::NONE) : loc(loc) {
    update(name);
  }
  
  Identifier &operator=(const std::string& v) {
    update(v);
    return *this;
  }
  bool operator== (const Identifier& other) const {
    // This is just pointer equality between interned strings.
    return interned_name_ptr == other.interned_name_ptr;
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
  const std::string &getName() const { return *interned_name_ptr; }
  size_t getHash() const { return hash; }

  const char *c_str() const {
    return interned_name_ptr->c_str();
  }
  bool empty() const {
    return interned_name_ptr->empty();
  }
  const Location &location() const {
    return loc;
  }
  bool is_config_variable() const {
    return is_config_variable_;
  }

private:

  friend class Context;

  mutable boost::optional<const BuiltinFunction *> resolved_builtin_function;

  void update(const std::string &name);

  bool is_config_variable_ = false;
  std::string *interned_name_ptr = NULL;
  size_t hash = 0;
  Location loc;

  static std::unordered_map<std::string, std::string> interned_names;
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

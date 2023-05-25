#pragma once
#include <string>
#include <boost/optional.hpp>
#include "AST.h"

class BuiltinFunction;

class Identifier
{
public:
  Identifier() : name(""), loc(Location::NONE) {
    update_hash();
  }
  Identifier(const std::string &name, const Location &loc) : name(name), loc(loc) {
    update_hash();
  }
  Identifier(const char *name, const Location &loc = Location::NONE) : name(name), loc(loc) {
    update_hash();
  }
  explicit Identifier(std::string &&name, const Location &loc) : name(std::move(name)), loc(loc) {
    update_hash();
  }
  
  Identifier &operator=(const std::string& v) {
    name = v;
    update_hash();
    return *this;
  }
  bool operator== (const Identifier& other) const {
    return hash == other.hash && name == other.name;
  }
  bool operator<(const Identifier& other) const {
    return name < other.name;
  }
  bool operator== (const char *other) const {
    return name == other;
  }
  bool operator== (const std::string &other) const {
    return name == other;
  }
  operator const std::string&() const { return name; }
  const std::string &getName() const { return name; }
  size_t getHash() const { return hash; }

  const char *c_str() const {
    return name.c_str();
  }
  bool empty() const {
    return name.empty();
  }
  const Location &location() const {
    return loc;
  }

private:

  friend class Context;

  mutable boost::optional<const BuiltinFunction *> resolved_builtin_function;

  void update_hash() {
    hash = std::hash<std::string>()(name);
  }
  std::string name;
  size_t hash;
  Location loc;
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

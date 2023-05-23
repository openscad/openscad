#pragma once
#include <string>
#include <unordered_map>

class Identifier
{
public:
  Identifier() : name("") {
    update_hash();
  }
  Identifier(const std::string &name) : name(name) {
    update_hash();
  }
  Identifier(const char *name) : name(name) {
    update_hash();
  }
  explicit Identifier(std::string &&name) : name(std::move(name)) {
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
  operator const std::string&() const { return name; }
  const std::string &getName() const { return name; }
  size_t getHash() const { return hash; }

  const char *c_str() const {
    return name.c_str();
  }
  bool empty() const {
    return name.empty();
  }

private:

  void update_hash() {
    hash = std::hash<std::string>()(name);
  }
  std::string name;
  size_t hash;
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

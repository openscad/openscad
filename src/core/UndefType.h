#pragma once

#include <initializer_list>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

class Value;

/*
   Require a reason why (string), any time an undefined value is created/returned.
   This allows for passing of "exception" information from low level functions (i.e. from value.cc)
   up to Expression::evaluate() or other functions where a proper "WARNING: ..."
   with ASTNode Location info (source file, line number) can be included.
 */
class UndefType
{
public:
  // TODO: eventually deprecate undef creation without a reason.
  UndefType() : reasons{std::make_unique<std::vector<std::string>>()} { }
  explicit UndefType(const std::string& why) : reasons{std::make_unique<std::vector<std::string>>(std::initializer_list<std::string>({why}))} { }

  // Append another reason in case a chain of undefined operations are made before handling
  const UndefType& append(const std::string& why) const { reasons->push_back(why); return *this; }

  Value operator<(const UndefType& other) const;
  Value operator>(const UndefType& other) const;
  Value operator<=(const UndefType& other) const;
  Value operator>=(const UndefType& other) const;

  std::string toString() const {
    std::ostringstream stream;
    if (!reasons->empty()) {
      auto it = reasons->begin();
      stream << *it;
      for (++it; it != reasons->end(); ++it) {
        stream << "\n\t" << *it;
      }
    }
    // clear reasons so multiple same warnings are not given on the same value
    reasons->clear();
    return stream.str();
  }
  bool empty() const { return reasons->empty(); }
private:
  // using unique_ptr to keep the size small enough that the variant of
  // all value types does not exceed the 24 bytes.
  // mutable to allow clearing reasons, which should avoid duplication of warnings that have already been displayed.
  mutable std::unique_ptr<std::vector<std::string>> reasons;
};


inline std::ostream& operator<<(std::ostream& stream, const UndefType& /*u*/)
{
  stream << "undef";
  return stream;
}

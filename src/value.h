#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <iostream>
#include <memory>

// Workaround for https://bugreports.qt-project.org/browse/QTBUG-22829
#ifndef Q_MOC_RUN
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#endif

#include "Assignment.h"
#include "memory.h"

class tostring_visitor;
class tostream_visitor;
class Context;
class Expression;

class QuotedString : public std::string
{
public:
  QuotedString() : std::string() {}
  QuotedString(const std::string &s) : std::string(s) {}
};
std::ostream &operator<<(std::ostream &stream, const QuotedString &s);

class Filename : public QuotedString
{
public:
  Filename() : QuotedString() {}
  Filename(const std::string &f) : QuotedString(f) {}
};
std::ostream &operator<<(std::ostream &stream, const Filename &filename);

class RangeType {
private:
  double begin_val;
  double step_val;
  double end_val;

public:
  static constexpr uint32_t MAX_RANGE_STEPS = 10000;
  static const RangeType EMPTY;

  enum class type_t { RANGE_TYPE_BEGIN, RANGE_TYPE_RUNNING, RANGE_TYPE_END };

  class iterator {
  public:
    // iterator_traits required types:
    using iterator_category = std::forward_iterator_tag ;
    using value_type        = double;
    using difference_type   = void;       // type used by operator-(iterator), not implemented for forward iterator
    using reference         = value_type; // type used by operator*(), not actually a reference
    using pointer           = void;       // type used by operator->(), not implemented
    iterator(const RangeType &range, type_t type);
    iterator& operator++();
    reference operator*();
    bool operator==(const iterator& other) const;
    bool operator!=(const iterator& other) const;
  private:
    const RangeType &range;
    double val;
    type_t type;
    const uint32_t num_values;
    uint32_t i_step;
    void update_type();
  };

  RangeType(const RangeType &) = delete;            // never copy, move instead
  RangeType& operator=(const RangeType &) = delete; // never copy, move instead
  RangeType(RangeType&&) = default;
  RangeType& operator=(RangeType&&) = default;

  explicit RangeType(double begin, double end)
    : begin_val(begin), step_val(1.0), end_val(end) {}

  explicit RangeType(double begin, double step, double end)
    : begin_val(begin), step_val(step), end_val(end) {}

  bool operator==(const RangeType &other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n1 == 0) return n2 == 0;
    if (n2 == 0) return false;
    return this == &other ||
      (this->begin_val == other.begin_val &&
       this->step_val == other.step_val &&
       n1 == n2);
  }

  bool operator!=(const RangeType &other) const {
    return !(*this==other);
  }

  bool operator<(const RangeType &other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n1 == 0) return 0 < n2;
    if (n2 == 0) return false;
    return this->begin_val < other.begin_val ||
      (this->begin_val == other.begin_val &&
        (this->step_val < other.step_val || (this->step_val == other.step_val && n1 < n2))
      );
  }

  bool operator<=(const RangeType &other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n1 == 0) return true; // (0 <= n2) is always true
    if (n2 == 0) return false;
    return this->begin_val < other.begin_val ||
      (this->begin_val == other.begin_val &&
        (this->step_val < other.step_val || (this->step_val == other.step_val && n1 <= n2))
      );
  }

  bool operator>(const RangeType &other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n2 == 0) return n1 > 0;
    if (n1 == 0) return false;
    return this->begin_val > other.begin_val ||
      (this->begin_val == other.begin_val &&
        (this->step_val > other.step_val || (this->step_val == other.step_val && n1 > n2))
      );
  }

  bool operator>=(const RangeType &other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n2 == 0) return true; // (n1 >= 0) is always true
    if (n1 == 0) return false;
    return this->begin_val > other.begin_val ||
      (this->begin_val == other.begin_val &&
        (this->step_val > other.step_val || (this->step_val == other.step_val && n1 >= n2))
      );
  }

  double begin_value() const { return begin_val; }
  double step_value() const { return step_val; }
  double end_value() const { return end_val; }

  iterator begin() const { return iterator(*this, type_t::RANGE_TYPE_BEGIN); }
  iterator end() const{ return iterator(*this, type_t::RANGE_TYPE_END); }

  /// return number of values, max uint32_t value if step is 0 or range is infinite
  uint32_t numValues() const;
};
std::ostream& operator<<(std::ostream& stream, const RangeType& r);


template <typename T>
class ValuePtr {
private:
  explicit ValuePtr(const std::shared_ptr<T> &val_in) : value(val_in) { }
public:
  ValuePtr(T&& value) : value(std::make_shared<T>(std::move(value))) { }
  ValuePtr clone() const { return ValuePtr(value); }

  const T& operator*() const { return *value; }
  const T* operator->() const { return value.get(); }

private:
  std::shared_ptr<T> value;
};

using RangePtr = ValuePtr<RangeType>;

class str_utf8_wrapper
{
private:
  // store the cached length in glong, paired with its string
  struct str_utf8_t {
    static constexpr glong LENGTH_UNKNOWN = -1;
    str_utf8_t() : u8str(), u8len(0) { }
    str_utf8_t(const std::string& s) : u8str(s) { }
    str_utf8_t(const char* cstr) : u8str(cstr) { }
    str_utf8_t(const char* cstr, size_t size, glong u8len) : u8str(cstr, size), u8len(u8len) { }
    const std::string u8str;
    glong u8len = LENGTH_UNKNOWN;
  };
  // private constructor for copying members
  explicit str_utf8_wrapper(const shared_ptr<str_utf8_t> &str_in) : str_ptr(str_in) { }

public:
  class iterator {
  public:
    // iterator_traits required types:
    using iterator_category = std::forward_iterator_tag ;
    using value_type        = str_utf8_wrapper;
    using difference_type   = void;
    using reference         = value_type; // type used by operator*(), not actually a reference
    using pointer           = void;
    iterator() : ptr(&nullterm) {} // DefaultConstructible
    iterator(const str_utf8_wrapper& str) : ptr(str.c_str()), len(char_len()) { }
    iterator(const str_utf8_wrapper& str, bool /*end*/) : ptr(str.c_str() + str.size()) { }

    iterator& operator++() { ptr += len; len = char_len(); return *this; }
    reference operator*() { return {ptr, len}; } // Note: returns a new str_utf8_wrapper **by value**, representing a single UTF8 character.
    bool operator==(const iterator &other) const { return ptr == other.ptr; }
    bool operator!=(const iterator &other) const { return ptr != other.ptr; }
  private:
    size_t char_len();
    static const char nullterm = '\0';
    const char *ptr;
    size_t len = 0;
  };

  iterator begin() const { return iterator(*this); }
  iterator end() const{ return iterator(*this, true); }
  str_utf8_wrapper() : str_ptr(make_shared<str_utf8_t>()) { }
  str_utf8_wrapper(const std::string& s) : str_ptr(make_shared<str_utf8_t>(s)) { }
  str_utf8_wrapper(const char* cstr) : str_ptr(make_shared<str_utf8_t>(cstr)) { }
  // for enumerating single utf8 chars from iterator
  str_utf8_wrapper(const char* cstr, size_t clen) : str_ptr(make_shared<str_utf8_t>(cstr,clen,1)) { }
  str_utf8_wrapper(const str_utf8_wrapper &) = delete; // never copy, move instead
  str_utf8_wrapper& operator=(const str_utf8_wrapper &) = delete; // never copy, move instead
  str_utf8_wrapper(str_utf8_wrapper&&) = default;
  str_utf8_wrapper& operator=(str_utf8_wrapper&&) = default;
  str_utf8_wrapper clone() const { return str_utf8_wrapper(this->str_ptr); } // makes a copy of shared_ptr

  bool operator==(const str_utf8_wrapper &rhs) const { return this->str_ptr->u8str == rhs.str_ptr->u8str; }
  bool operator!=(const str_utf8_wrapper &rhs) const { return this->str_ptr->u8str != rhs.str_ptr->u8str; }
  bool operator< (const str_utf8_wrapper &rhs) const { return this->str_ptr->u8str <  rhs.str_ptr->u8str; }
  bool operator> (const str_utf8_wrapper &rhs) const { return this->str_ptr->u8str >  rhs.str_ptr->u8str; }
  bool operator<=(const str_utf8_wrapper &rhs) const { return this->str_ptr->u8str <= rhs.str_ptr->u8str; }
  bool operator>=(const str_utf8_wrapper &rhs) const { return this->str_ptr->u8str >= rhs.str_ptr->u8str; }
  bool empty() const { return this->str_ptr->u8str.empty(); }
  const char* c_str() const { return this->str_ptr->u8str.c_str(); }
  const std::string& toString() const { return this->str_ptr->u8str; }
  size_t size() const { return this->str_ptr->u8str.size(); }

  glong get_utf8_strlen() const {
    if (str_ptr->u8len == str_utf8_t::LENGTH_UNKNOWN) {
      str_ptr->u8len = g_utf8_strlen(str_ptr->u8str.c_str(), str_ptr->u8str.size());
    }
    return str_ptr->u8len;
  };

private:
  shared_ptr<str_utf8_t> str_ptr;
};

class FunctionType {
public:
  FunctionType(std::shared_ptr<Context> ctx, std::shared_ptr<Expression> expr, std::shared_ptr<AssignmentList> args)
    : ctx(ctx), expr(expr), args(args) { }
  Value operator==(const FunctionType &other) const;
  Value operator!=(const FunctionType &other) const;
  Value operator< (const FunctionType &other) const;
  Value operator> (const FunctionType &other) const;
  Value operator<=(const FunctionType &other) const;
  Value operator>=(const FunctionType &other) const;

  const std::shared_ptr<Context>& getCtx() const { return ctx; }
  const std::shared_ptr<Expression>& getExpr() const { return expr; }
  const std::shared_ptr<AssignmentList>& getArgs() const { return args; }
private:
  std::shared_ptr<Context> ctx;
  std::shared_ptr<Expression> expr;
  std::shared_ptr<AssignmentList> args;
};

using FunctionPtr = ValuePtr<FunctionType>;

std::ostream& operator<<(std::ostream& stream, const FunctionType& f);

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
  UndefType() : reasons{ std::make_unique<std::vector<std::string>>() } { }
  explicit UndefType(const std::string &why) : reasons{ std::make_unique<std::vector<std::string>>(std::initializer_list<std::string>({why})) } { }

  // Append another reason in case a chain of undefined operations are made before handling
  const UndefType &append(const std::string &why) const { reasons->push_back(why); return *this; }

  Value operator< (const UndefType &other) const;
  Value operator> (const UndefType &other) const;
  Value operator<=(const UndefType &other) const;
  Value operator>=(const UndefType &other) const;
  friend std::ostream& operator<<(std::ostream& stream, const ValuePtr<UndefType>& u);

  std::string toString() const;
  bool empty() const { return reasons->empty(); }
private:
  // using unique_ptr to keep the size small enough that the variant of
  // all value types does not exceed the 24 bytes.
  // mutable to allow clearing reasons, which should avoid duplication of warnings that have already been displayed.
  mutable std::unique_ptr<std::vector<std::string>> reasons;
};

std::ostream& operator<<(std::ostream& stream, const UndefType& u);

/**
 *  Value class encapsulates a boost::variant value which can represent any of the
 *  value types existing in the SCAD language.
 * -- As part of a refactoring effort which began as PR #2881 and continued as PR #3102,
 *    Value and its constituent types have been made (nominally) "move only".
 * -- In some cases a copy of a Value is necessary or unavoidable, in which case Value::clone() can be used.
 * -- Value::clone() is used instead of automatic copy construction/assignment so this action is
 *    made deliberate and explicit (and discouraged).
 * -- Recommended to make use of RVO (Return Value Optimization) wherever possible:
 *       https://en.cppreference.com/w/cpp/language/copy_elision
 * -- Classes which cache Values such as Context or dxf_dim_cache(see dxfdim.cc), when queried
 *    should return either a const reference or a clone of the cached value if returning by-value.
 *    NEVER return a non-const reference!
 */
class Value
{
public:
  enum class Type {
    UNDEFINED,
    BOOL,
    NUMBER,
    STRING,
    VECTOR,
    EMBEDDED_VECTOR,
    RANGE,
    FUNCTION
  };
  // FIXME: eventually remove this in favor of specific messages for each undef usage
  static const Value undefined;

  /**
   * VectorType is the underlying "BoundedType" of boost::variant for OpenSCAD vectors.
   * It holds only a shared_ptr to its VectorObject type, and provides a convenient
   * interface for various operations needed on the vector.
   *
   * EmbeddedVectorType class derives from VectorType and enables O(1) concatenation of vectors
   * by treating their elements as elements of their parent, traversable via VectorType's custom iterator.
   * -- An embedded vector should never exist "in the wild", only as a pseudo-element of a parent vector.
   *    Eg "Lc*" Expressions return Embedded Vectors but they are necessairly child expressions of a Vector expression.
   * -- Any VectorType containing embedded elements will be forced to "flatten" upon usage of operator[],
   *    which is the only case of random-access.
   * -- Any loops through VectorTypes should prefer automatic range-based for loops  eg: for(const auto& value : vec) { ... }
   *    which make use of begin() and end() iterators of VectorType.  https://en.cppreference.com/w/cpp/language/range-for
   * -- Moving a temporary Value of type VectorType or EmbeddedVectorType is always safe,
   *    since it just moves the shared_ptr in its possession (which might be a copy but that doesn't matter).
   *    Additionally any VectorType can be converted to an EmbeddedVectorType by moving it into
   *    EmbeddedVectorType's converting constructor (or vice-versa).
   * -- HOWEVER, moving elements out of a [Embedded]VectorType is potentially DANGEROUS unless it can be
   *    verified that ( ptr.use_count() == 1 ) for that outermost [Embedded]VectorType
   *    AND recursively any EmbeddedVectorTypes which led to that element.
   *    Therefore elements are currently cloned rather than making any attempt to move.
   *    Performing such use_count checks may be an area for further optimization.
   */
  class EmbeddedVectorType;
  class VectorType {

  protected:
    // The object type which VectorType's shared_ptr points to.
    struct VectorObject {
      using vec_t = std::vector<Value>;
      using size_type = vec_t::size_type;
      vec_t vec;
      size_type embed_excess = 0; // Keep count of the number of embedded elements *excess of* vec.size()
      size_type size() const { return vec.size() + embed_excess;  }
    };
    using vec_t = VectorObject::vec_t;
    shared_ptr<VectorObject> ptr;

    // A Deleter is used on the shared_ptrs to avoid stack overflow in cases
    // of destructing a very large list of nested embedded vectors, such as from a
    // recursive function which concats one element at a time.
    // (A similar solution can also be seen with csgnode.h:CSGOperationDeleter).
    struct VectorObjectDeleter {
      void operator()(VectorObject* vec);
    };
    void flatten() const; // flatten replaces VectorObject::vec with a new vector
                          // where any embedded elements are copied directly into the top level vec,
                          // leaving only true elements for straightforward indexing by operator[].
    explicit VectorType(const shared_ptr<VectorObject> &copy) : ptr(copy) { } // called by clone()
  public:
    using size_type = VectorObject::size_type;
    static const VectorType EMPTY;
    // EmbeddedVectorType-aware iterator, manages its own stack of begin/end vec_t::const_iterators
    // such that calling code will only receive references to "true" elements (i.e. NOT EmbeddedVectorTypes).
    // Also tracks the overall element index. In case flattening occurs during iteration, it can continue based on that index. (Issue #3541)
    class iterator {
    private:
      const VectorObject* vo;
      std::vector<std::pair<vec_t::const_iterator, vec_t::const_iterator> > it_stack;
      vec_t::const_iterator it, end;
      size_t index;

      // Recursively push stack while current (pseudo)element is an EmbeddedVector
      //  - Depends on the fact that VectorType::emplace_back(EmbeddedVectorType&& mbed)
      //    will not embed an empty vector, which ensures iterator will arrive at an actual element,
      //    unless already at end of parent VectorType.
      void check_and_push()
      {
        if (it != end) {
          while (it->type() == Type::EMBEDDED_VECTOR) {
            const vec_t &cur = it->toEmbeddedVector().ptr->vec;
            it_stack.emplace_back(it, end);
            it = cur.begin();
            end = cur.end();
          }
        }
      }
    public:
      using iterator_category = std::forward_iterator_tag ;
      using value_type        = Value;
      using difference_type   = void;
      using reference         = const value_type&;
      using pointer           = const value_type*;

      iterator() : vo(EMPTY.ptr.get()), it_stack(), it(EMPTY.ptr->vec.begin()), end(EMPTY.ptr->vec.end()), index(0) {}
      iterator(const VectorObject* v) : vo(v), it(v->vec.begin()), end(v->vec.end()), index(0) {
        if (vo->embed_excess) check_and_push();
      }
      iterator(const VectorObject* v, bool /*end*/) : vo(v), index(v->size()) { }
      iterator& operator++() {
        ++index;
        if (vo->embed_excess) {
          // recursively increment and pop stack while at the end of EmbeddedVector(s)
          while (++it == end && !it_stack.empty()) {
            const auto& up = it_stack.back();
            it = up.first;
            end = up.second;
            it_stack.pop_back();
          }
          check_and_push();
        } else { // vo->vec is flat
          it = vo->vec.begin() + index;
        }
        return *this;
      }
      reference operator*() const { return *it; };
      pointer operator->() const { return &*it; };
      bool operator==(const iterator &other) const { return this->vo == other.vo && this->index == other.index; }
      bool operator!=(const iterator &other) const { return this->vo != other.vo || this->index != other.index; }
    };
    using const_iterator = const iterator;
    VectorType() : ptr(shared_ptr<VectorObject>(new VectorObject(), VectorObjectDeleter() )) {}
    VectorType(double x, double y, double z);
    VectorType(const VectorType &) = delete;            // never copy, move instead
    VectorType& operator=(const VectorType &) = delete; // never copy, move instead
    VectorType(VectorType&&) = default;
    VectorType& operator=(VectorType&&) = default;
    VectorType clone() const { return VectorType(this->ptr); } // Copy explicitly only when necessary
    static Value Empty() { return VectorType(); }

    const_iterator begin() const { return iterator(ptr.get()); }
    const_iterator   end() const { return iterator(ptr.get(), true); }
    size_type size() const { return ptr->size(); }
    bool empty() const { return ptr->vec.empty(); }
    // const accesses to VectorObject require .clone to be move-able
    const Value &operator[](size_t idx) const {
      if (idx < this->size()) {
        if (ptr->embed_excess) flatten();
         return ptr->vec[idx];
      } else {
        return Value::undefined;
      }
    }
    Value operator==(const VectorType &v) const;
    Value operator< (const VectorType &v) const;
    Value operator> (const VectorType &v) const;
    Value operator!=(const VectorType &v) const;
    Value operator<=(const VectorType &v) const;
    Value operator>=(const VectorType &v) const;

    void emplace_back(Value&& val);
    void emplace_back(EmbeddedVectorType&& mbed);
    template<typename... Args> void emplace_back(Args&&... args) { ptr->vec.emplace_back(std::forward<Args>(args)...); }
  };

  class EmbeddedVectorType : public VectorType {
  private:
      explicit EmbeddedVectorType(const shared_ptr<VectorObject> &copy) : VectorType(copy) { } // called by clone()
  public:
    EmbeddedVectorType() : VectorType() {};
    EmbeddedVectorType(const EmbeddedVectorType &) = delete;
    EmbeddedVectorType& operator=(const EmbeddedVectorType &) = delete;
    EmbeddedVectorType(EmbeddedVectorType&&) = default;
    EmbeddedVectorType& operator=(EmbeddedVectorType&&) = default;

    EmbeddedVectorType(VectorType&& v) : VectorType(std::move(v)) {}; // converting constructor
    EmbeddedVectorType clone() const { return EmbeddedVectorType(this->ptr); }
    static Value Empty() { return EmbeddedVectorType(); }
  };

private:
  Value() : value(UndefType()) { } // Don't default construct empty Values.  If "undefined" needed, use reference to Value::undefined, or call Value::undef() for return by value
public:
  Value(const Value &) = delete; // never copy, move instead
  Value &operator=(const Value &v) = delete; // never copy, move instead
  Value(Value&&) = default;
  Value& operator=(Value&&) = default;
  Value clone() const; // Use sparingly to explicitly copy a Value

  Value(int v) : value(double(v)) { }
  Value(const char *v) : value(str_utf8_wrapper(v)) { } // prevent insane implicit conversion to bool!
  Value(      char *v) : value(str_utf8_wrapper(v)) { } // prevent insane implicit conversion to bool!
                                                        // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0608r3.html
  template<class T> Value(T&& val) : value(std::forward<T>(val)) { }

  static Value undef(const std::string &why); // creation of undef requires a reason!

  const std::string typeName() const;
  Type type() const { return static_cast<Type>(this->value.which()); }
  bool isDefinedAs(const Type type) const { return this->type() == type; }
  bool isDefined()   const { return this->type() != Type::UNDEFINED; }
  bool isUndefined() const { return this->type() == Type::UNDEFINED; }
  bool isUncheckedUndef() const;

  // Conversion to boost::variant "BoundedType"s. const ref where appropriate.
  bool toBool() const;
  double toDouble() const;
  const str_utf8_wrapper& toStrUtf8Wrapper() const;
  const VectorType &toVector() const;
  const EmbeddedVectorType& toEmbeddedVector() const;
  VectorType &toVectorNonConst();
  EmbeddedVectorType &toEmbeddedVectorNonConst();
  const RangeType& toRange() const;
  const FunctionType& toFunction() const;

  // Other conversion utility functions
  bool getDouble(double &v) const;
  bool getFiniteDouble(double &v) const;
  std::string toString() const;
  std::string toString(const tostring_visitor *visitor) const;
  std::string toEchoString() const;
  std::string toEchoString(const tostring_visitor *visitor) const;
  const UndefType& toUndef();
  std::string toUndefString() const;
  void toStream(std::ostringstream &stream) const;
  void toStream(const tostream_visitor *visitor) const;
  std::string chrString() const;
  bool getVec2(double &x, double &y, bool ignoreInfinite = false) const;
  bool getVec3(double &x, double &y, double &z) const;
  bool getVec3(double &x, double &y, double &z, double defaultval) const;

  // Common Operators
  operator bool() const = delete;
  Value operator==(const Value &v) const;
  Value operator!=(const Value &v) const;
  Value operator< (const Value &v) const;
  Value operator<=(const Value &v) const;
  Value operator>=(const Value &v) const;
  Value operator> (const Value &v) const;
  Value operator-() const;
  Value operator[](size_t idx) const;
  Value operator[](const Value &v) const;
  Value operator+(const Value &v) const;
  Value operator-(const Value &v) const;
  Value operator*(const Value &v) const;
  Value operator/(const Value &v) const;
  Value operator%(const Value &v) const;
  Value operator^(const Value &v) const;

  static bool cmp_less(const Value& v1, const Value& v2);

  friend std::ostream &operator<<(std::ostream &stream, const Value &value) {
    if (value.type() == Value::Type::STRING) stream << QuotedString(value.toString());
    else stream << value.toString();
    return stream;
  }

  typedef boost::variant<UndefType, bool, double, str_utf8_wrapper, VectorType, EmbeddedVectorType, RangePtr, FunctionPtr> Variant;
  static_assert(sizeof(Variant) <= 24, "Memory size of Value too big");

private:
  Variant value;
};

using VectorType = Value::VectorType;
using EmbeddedVectorType = Value::EmbeddedVectorType;

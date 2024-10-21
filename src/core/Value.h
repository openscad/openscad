#pragma once

#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
#include <algorithm>
#include <cstddef>
#include <ostream>
#include <memory>
#include <type_traits>
#include <variant>

#include "core/FunctionType.h"
#include "core/RangeType.h"
#include "core/str_utf8_wrapper.h"
#include "core/UndefType.h"

class tostring_visitor;
class tostream_visitor;
class Expression;
class Value;

class QuotedString : public std::string
{
public:
  QuotedString() : std::string() {}
  QuotedString(const std::string& s) : std::string(s) {}
};
std::ostream& operator<<(std::ostream& stream, const QuotedString& s);

class Filename : public QuotedString
{
public:
  Filename() : QuotedString() {}
  Filename(const std::string& f) : QuotedString(f) {}
};
std::ostream& operator<<(std::ostream& stream, const Filename& filename);

template <typename T>
class ValuePtr
{
private:
  explicit ValuePtr(std::shared_ptr<T> val_in) : value(std::move(val_in)) { }
public:
  ValuePtr(T&& value) : value(std::make_shared<T>(std::move(value))) { }
  [[nodiscard]] ValuePtr clone() const { return ValuePtr(value); }

  const T& operator*() const { return *value; }
  const T *operator->() const { return value.get(); }
  [[nodiscard]] const std::shared_ptr<T>& get() const { return value; }

private:
  std::shared_ptr<T> value;
};

using RangePtr = ValuePtr<RangeType>;
using FunctionPtr = ValuePtr<FunctionType>;

/**
 *  Value class encapsulates a std::variant value which can represent any of the
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
    FUNCTION,
    OBJECT
  };
  // FIXME: eventually remove this in favor of specific messages for each undef usage
  static const Value undefined;

  /**
   * VectorType is the underlying "BoundedType" of std::variant for OpenSCAD vectors.
   * It holds only a shared_ptr to its VectorObject type, and provides a convenient
   * interface for various operations needed on the vector.
   *
   * EmbeddedVectorType class derives from VectorType and enables O(1) concatenation of vectors
   * by treating their elements as elements of their parent, traversable via VectorType's custom iterator.
   * -- An embedded vector should never exist "in the wild", only as a pseudo-element of a parent vector.
   *    Eg "Lc*" Expressions return Embedded Vectors but they are necessarily child expressions of a Vector expression.
   * -- Any VectorType containing embedded elements will be forced to "flatten" upon usage of operator[],
   *    which is the only case of random-access.
   * -- Any loops through VectorTypes should prefer automatic range-based for loops eg: for(const auto& value : vec) { ... }
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
  class VectorType
  {

protected:
    // The object type which VectorType's shared_ptr points to.
    struct VectorObject {
      using vec_t = std::vector<Value>;
      using size_type = vec_t::size_type;
      vec_t vec;
      size_type embed_excess = 0; // Keep count of the number of embedded elements *excess of* vec.size()
      class EvaluationSession *evaluation_session = nullptr; // Used for heap size bookkeeping. May be null for vectors of known small maximum size.
      [[nodiscard]] size_type size() const { return vec.size() + embed_excess;  }
      [[nodiscard]] bool empty() const { return vec.empty() && embed_excess == 0;  }
    };
    using vec_t = VectorObject::vec_t;
public:
    std::shared_ptr<VectorObject> ptr;
protected:

    // A Deleter is used on the shared_ptrs to avoid stack overflow in cases
    // of destructing a very large list of nested embedded vectors, such as from a
    // recursive function which concats one element at a time.
    // (A similar solution can also be seen with CSGNode.h:CSGOperationDeleter).
    struct VectorObjectDeleter {
      void operator()(VectorObject *vec);
    };
    void flatten() const; // flatten replaces VectorObject::vec with a new vector
                          // where any embedded elements are copied directly into the top level vec,
                          // leaving only true elements for straightforward indexing by operator[].
    explicit VectorType(const std::shared_ptr<VectorObject>& copy) : ptr(copy) { } // called by clone()
public:
    using size_type = VectorObject::size_type;
    static const VectorType EMPTY;
    // EmbeddedVectorType-aware iterator, manages its own stack of begin/end vec_t::const_iterators
    // such that calling code will only receive references to "true" elements (i.e. NOT EmbeddedVectorTypes).
    // Also tracks the overall element index. In case flattening occurs during iteration, it can continue based on that index. (Issue #3541)
    class iterator
    {
private:
      const VectorObject *vo;
      std::vector<std::pair<vec_t::const_iterator, vec_t::const_iterator>> it_stack;
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
            const vec_t& cur = it->toEmbeddedVector().ptr->vec;
            it_stack.emplace_back(it, end);
            it = cur.begin();
            end = cur.end();
          }
        }
      }
public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = Value;
      using difference_type = void;
      using reference = const value_type&;
      using pointer = const value_type *;

      iterator() : vo(EMPTY.ptr.get()), it_stack(), it(EMPTY.ptr->vec.begin()), end(EMPTY.ptr->vec.end()), index(0) {}
      iterator(const VectorObject *v) : vo(v), it(v->vec.begin()), end(v->vec.end()), index(0) {
        if (vo->embed_excess) check_and_push();
      }
      iterator(const VectorObject *v, bool /*end*/) : vo(v), index(v->size()) { }
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
          it = vo->vec.begin() + static_cast<vec_t::iterator::difference_type>(index);
        }
        return *this;
      }
      reference operator*() const { return *it; }
      pointer operator->() const { return &*it; }
      bool operator==(const iterator& other) const { return this->vo == other.vo && this->index == other.index; }
      bool operator!=(const iterator& other) const { return this->vo != other.vo || this->index != other.index; }
    };
    using const_iterator = const iterator;
    VectorType(class EvaluationSession *session);
    VectorType(class EvaluationSession *session, double x, double y, double z);
    VectorType(const VectorType&) = delete; // never copy, move instead
    VectorType& operator=(const VectorType&) = delete; // never copy, move instead
    VectorType(VectorType&&) = default;
    VectorType& operator=(VectorType&&) = default;
    ~VectorType() = default;
    [[nodiscard]] VectorType clone() const { return VectorType(this->ptr); } // Copy explicitly only when necessary
    static Value Empty() { return VectorType(nullptr); }

    void reserve(size_t size) {
      ptr->vec.reserve(size);
    }

    [[nodiscard]] const_iterator begin() const { return iterator(ptr.get()); }
    [[nodiscard]] const_iterator   end() const { return iterator(ptr.get(), true); }
    [[nodiscard]] size_type size() const { return ptr->size(); }
    [[nodiscard]] bool empty() const { return ptr->empty(); }
    // const accesses to VectorObject require .clone to be move-able
    const Value& operator[](size_t idx) const {
      if (idx < this->size()) {
        if (ptr->embed_excess) flatten();
        return ptr->vec[idx];
      } else {
        return Value::undefined;
      }
    }
    Value operator==(const VectorType& v) const;
    Value operator<(const VectorType& v) const;
    Value operator>(const VectorType& v) const;
    Value operator!=(const VectorType& v) const;
    Value operator<=(const VectorType& v) const;
    Value operator>=(const VectorType& v) const;
    [[nodiscard]] class EvaluationSession *evaluation_session() const { return ptr->evaluation_session; }

    void emplace_back(Value&& val);
    void emplace_back(EmbeddedVectorType&& mbed);
    template <typename ... Args> void emplace_back(Args&&... args) { emplace_back(Value(std::forward<Args>(args)...)); }
  };

  class EmbeddedVectorType : public VectorType
  {
private:
    explicit EmbeddedVectorType(const std::shared_ptr<VectorObject>& copy) : VectorType(copy) { } // called by clone()
public:
    EmbeddedVectorType(class EvaluationSession *session) : VectorType(session) {}
    EmbeddedVectorType(const EmbeddedVectorType&) = delete;
    EmbeddedVectorType& operator=(const EmbeddedVectorType&) = delete;
    EmbeddedVectorType(EmbeddedVectorType&&) = default;
    EmbeddedVectorType& operator=(EmbeddedVectorType&&) = default;
    ~EmbeddedVectorType() = default;
    EmbeddedVectorType(VectorType&& v) : VectorType(std::move(v)) {} // converting constructor
    [[nodiscard]] EmbeddedVectorType clone() const { return EmbeddedVectorType(this->ptr); }
    static Value Empty() { return EmbeddedVectorType(nullptr); }
  };

  class ObjectType
  {
protected:
    struct ObjectObject;
    struct ObjectObjectDeleter {
      void operator()(ObjectObject *obj);
    };

private:
    explicit ObjectType(const std::shared_ptr<ObjectObject>& copy);

public:
    std::shared_ptr<ObjectObject> ptr;
    ObjectType(class EvaluationSession *session);
    [[nodiscard]] ObjectType clone() const;
    [[nodiscard]] const Value& get(const std::string& key) const;
    void set(const std::string& key, Value&& value);
    Value operator==(const ObjectType& v) const;
    Value operator<(const ObjectType& v) const;
    Value operator>(const ObjectType& v) const;
    Value operator!=(const ObjectType& v) const;
    Value operator<=(const ObjectType& v) const;
    Value operator>=(const ObjectType& v) const;
    const Value& operator[](const str_utf8_wrapper& v) const;
    [[nodiscard]] const std::vector<std::string>& keys() const;
  };

private:
  Value() : value(UndefType()) { } // Don't default construct empty Values.  If "undefined" needed, use reference to Value::undefined, or call Value::undef() for return by value
public:
  Value(const Value&) = delete; // never copy, move instead
  Value& operator=(const Value& v) = delete; // never copy, move instead
  Value(Value&&) = default;
  Value& operator=(Value&&) = default;
  [[nodiscard]] Value clone() const; // Use sparingly to explicitly copy a Value
  ~Value() = default;

  Value(int v) : value(double(v)) { }
  Value(const char *v) : value(str_utf8_wrapper(v)) { } // prevent insane implicit conversion to bool!
  Value(char *v) : value(str_utf8_wrapper(v)) { } // prevent insane implicit conversion to bool!
                                                  // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0608r3.html
  // Don't shadow move constructor
  template <class T, class = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Value>>>
  Value(T&& val) : value(std::forward<T>(val)) { }

  static Value undef(const std::string& why); // creation of undef requires a reason!

  [[nodiscard]] const std::string typeName() const;
  [[nodiscard]] static std::string typeName(Type type);
  [[nodiscard]] Type type() const { return static_cast<Type>(this->value.index()); }
  [[nodiscard]] bool isDefinedAs(const Type type) const { return this->type() == type; }
  [[nodiscard]] bool isDefined()   const { return this->type() != Type::UNDEFINED; }
  [[nodiscard]] bool isUndefined() const { return this->type() == Type::UNDEFINED; }
  [[nodiscard]] bool isUncheckedUndef() const;

  // Conversion to std::variant "BoundedType"s. const ref where appropriate.
  [[nodiscard]] bool toBool() const;
  [[nodiscard]] double toDouble() const;
  [[nodiscard]] const str_utf8_wrapper& toStrUtf8Wrapper() const;
  [[nodiscard]] const VectorType& toVector() const;
  [[nodiscard]] const EmbeddedVectorType& toEmbeddedVector() const;
  [[nodiscard]] VectorType& toVectorNonConst();
  [[nodiscard]] EmbeddedVectorType& toEmbeddedVectorNonConst();
  [[nodiscard]] const RangeType& toRange() const;
  [[nodiscard]] const FunctionType& toFunction() const;
  [[nodiscard]] const ObjectType& toObject() const;

  // Other conversion utility functions
  bool getDouble(double& v) const;
  bool getFiniteDouble(double& v) const;
  bool getUnsignedInt(unsigned int& v) const;
  bool getPositiveInt(unsigned int& v) const;
  [[nodiscard]] std::string toString() const;
  [[nodiscard]] std::string toEchoString() const;
  [[nodiscard]] std::string toEchoStringNoThrow() const; //use this for warnings
  [[nodiscard]] const UndefType& toUndef() const;
  [[nodiscard]] std::string toUndefString() const;
  [[nodiscard]] std::string chrString() const;
  bool getVec2(double& x, double& y, bool ignoreInfinite = false) const;
  bool getVec3(double& x, double& y, double& z) const;
  bool getVec3(double& x, double& y, double& z, double defaultval) const;

  // Common Operators
  operator bool() const = delete;
  Value operator==(const Value& v) const;
  Value operator!=(const Value& v) const;
  Value operator<(const Value& v) const;
  Value operator<=(const Value& v) const;
  Value operator>=(const Value& v) const;
  Value operator>(const Value& v) const;
  Value operator-() const;
  Value operator[](size_t idx) const;
  Value operator[](const Value& v) const;
  Value operator+(const Value& v) const;
  Value operator-(const Value& v) const;
  Value operator*(const Value& v) const;
  Value operator/(const Value& v) const;
  Value operator%(const Value& v) const;
  Value operator^(const Value& v) const;

  static bool cmp_less(const Value& v1, const Value& v2);

  friend std::ostream& operator<<(std::ostream& stream, const Value& value) {
    if (value.type() == Value::Type::STRING) stream << QuotedString(value.toString());
    else stream << value.toString();
    return stream;
  }

  using Variant = std::variant<UndefType, bool, double, str_utf8_wrapper, VectorType, EmbeddedVectorType, RangePtr, FunctionPtr, ObjectType>;

  static_assert(sizeof(Value::Variant) <= 24, "Memory size of Value too big");
  [[nodiscard]] const Variant& getVariant() const { return value; }

private:
  Variant value;
};

// The object type which ObjectType's shared_ptr points to.
struct Value::ObjectType::ObjectObject {
  using obj_t = std::unordered_map<std::string, Value>;
  obj_t map;
  class EvaluationSession *evaluation_session = nullptr;
  std::vector<std::string> keys;
  std::vector<Value> values;
};

std::ostream& operator<<(std::ostream& stream, const Value::ObjectType& u);

using VectorType = Value::VectorType;
using EmbeddedVectorType = Value::EmbeddedVectorType;
using ObjectType = Value::ObjectType;

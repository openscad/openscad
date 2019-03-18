#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <iostream>

// Workaround for https://bugreports.qt-project.org/browse/QTBUG-22829
#ifndef Q_MOC_RUN
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#endif
#include <cstdint>
#include "memory.h"

class tostring_visitor;
class tostream_visitor;

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
	
	/// inverse begin/end if begin is upper than end
	void normalize();
	
public:
	enum class type_t { RANGE_TYPE_BEGIN, RANGE_TYPE_RUNNING, RANGE_TYPE_END };
  
	class iterator {
	public:
		typedef iterator self_type;
		typedef double value_type;
		typedef double& reference;
		typedef double* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef double difference_type;
		iterator(RangeType &range, type_t type);
		self_type operator++();
		self_type operator++(int junk);
		reference operator*();
		pointer operator->();
		bool operator==(const self_type& other) const;
		bool operator!=(const self_type& other) const;
	private:
		RangeType &range;
		double val;
		type_t type;
    
		void update_type();
	};
  
	RangeType(double begin, double end)
		: begin_val(begin), step_val(1.0), end_val(end)
    {
      normalize();
    }
	
	RangeType(double begin, double step, double end)
		: begin_val(begin), step_val(step), end_val(end) {}
	
	bool operator==(const RangeType &other) const {
		return this == &other ||
			(this->begin_val == other.begin_val &&
			 this->step_val == other.step_val &&
			 this->end_val == other.end_val);
	}
	
	double begin_value() { return begin_val; }
	double step_value() { return step_val; }
	double end_value() { return end_val; }
	
	iterator begin() { return iterator(*this, type_t::RANGE_TYPE_BEGIN); }
	iterator end() { return iterator(*this, type_t::RANGE_TYPE_END); }
	
	/// return number of values, max uint32_t value if step is 0 or range is infinite
	uint32_t numValues() const;
  
	friend class chr_visitor;
	friend class tostring_visitor;
	friend class tostream_visitor;
	friend class bracket_visitor;
};

class str_utf8_wrapper : public std::string
{
public:
	str_utf8_wrapper() : std::string(), cached_len(-1) { }
	explicit str_utf8_wrapper( const std::string& s ) : std::string( s ), cached_len(-1) { }
	explicit str_utf8_wrapper( size_t n, char c ) : std::string(n, c), cached_len(-1) { }
	~str_utf8_wrapper() {}
	
	glong get_utf8_strlen() const {
		if (cached_len < 0) {
			cached_len = g_utf8_strlen(this->c_str(), this->size());
		}
		return cached_len;
	};
private:
	mutable glong cached_len;
};

class Value
{
public:
	typedef std::vector<Value> VectorType;

  class VectorPtr {
  public:
    VectorPtr() : ptr(new VectorType()) {}
    VectorPtr(const VectorPtr &) = delete; // never copy, move instead
    VectorPtr &operator=(const VectorPtr &v) = delete; // never copy, move instead
    
    // move constructor & assignment
    VectorPtr(VectorPtr&& x) : ptr(std::move(x.ptr)) { x.ptr.reset(); };
    VectorPtr& operator=(VectorPtr&& x) { 
      ptr = std::move(x.ptr);
      x.ptr.reset();
      return *this;
    }
    // Copy explicitly only when necessary
    VectorPtr clone() const { VectorPtr c; c.ptr = this->ptr; return c;  }

    //VectorPtr(Value::VectorType &&); // do we want to me able to move VectorType?
    const Value::VectorType &operator*() const { return *ptr.get(); }
    Value::VectorType *operator->() const { return ptr.get(); }
    Value &operator[](size_t idx) const {	return (*ptr.get())[idx]; }
    bool operator==(const VectorPtr &v) const { return *ptr == *v; }
    bool operator!=(const VectorPtr &v) const {	return *ptr != *v; }
    operator bool() const {	return !ptr->empty(); }
    
    friend bool operator==(const VectorPtr &vptr, nullptr_t) { vptr.ptr == nullptr; };
    friend bool operator!=(const VectorPtr &vptr, nullptr_t) { vptr.ptr != nullptr; };
    friend bool operator==(nullptr_t, const VectorPtr &vptr) { nullptr == vptr.ptr; };
    friend bool operator!=(nullptr_t, const VectorPtr &vptr) { nullptr != vptr.ptr; };

  private:
    shared_ptr<Value::VectorType> ptr;
  };


public:

  enum class ValueType {
    UNDEFINED,
    BOOL,
    NUMBER,
    STRING,
    VECTOR,
    RANGE
  };
  static Value undefined() { return {}; }

  Value();
  Value(const Value &) = delete; // never copy, always move
  Value(Value&& x) : value(std::move(x.value))
  {
    x.value = boost::blank();
    //std::cout << "Value::Value(Value&&) " << *this << '\n';
  }
  explicit Value(bool v);
  explicit Value(int v);
  explicit Value(double v);
  Value(const std::string &v);
  explicit Value(const char *v);
  explicit Value(const char v);
  explicit Value(const RangeType &v);
  Value(VectorPtr&& v) : value(std::move(v)) {};

  Value clone() const; // Use sparingly to explicitly copy a Value

  ValueType type() const;
  bool isDefined() const;
  bool isDefinedAs(const ValueType type) const;
  bool isUndefined() const;

  double toDouble() const;
  bool getDouble(double &v) const;
  bool getFiniteDouble(double &v) const;
  bool toBool() const;
  std::string toString() const;
  const str_utf8_wrapper& toStrUtf8Wrapper();
  std::string toString(const tostring_visitor *visitor) const;
  std::string toEchoString() const;
  std::string toEchoString(const tostring_visitor *visitor) const;
  void toStream(std::ostringstream &stream) const;
  void toStream(const tostream_visitor *visitor) const;
  std::string chrString() const;
  const VectorPtr &toVectorPtr() const;

  bool getVec2(double &x, double &y, bool ignoreInfinite = false) const;
  bool getVec3(double &x, double &y, double &z) const;
  bool getVec3(double &x, double &y, double &z, double defaultval) const;
  RangeType toRange() const;

	operator bool() const { return this->toBool(); }

  Value &operator=(const Value &v) = delete; // never copy
  //Value& operator=(Value&&) = default;
  Value& operator=(Value&& x)
  {
    value = std::move(x.value);
    x.value = boost::blank();
    //std::cout << "Value::&operator=(Value&&) " << *this << '\n';
    return *this;
  }

  bool operator==(const Value &v) const;
  bool operator!=(const Value &v) const;
  bool operator<(const Value &v) const;
  bool operator<=(const Value &v) const;
  bool operator>=(const Value &v) const;
  bool operator>(const Value &v) const;
  Value operator-() const;
  //const Value &operator[](const Value &v) const;
  Value operator[](const Value &v) const;
  Value operator[](size_t idx) const;
  Value operator+(const Value &v) const;
  Value operator-(const Value &v) const;
  Value operator*(const Value &v) const;
  Value operator/(const Value &v) const;
  Value operator%(const Value &v) const;

  friend std::ostream &operator<<(std::ostream &stream, const Value &value) {
    if (value.type() == Value::ValueType::STRING) stream << QuotedString(value.toString());
    else stream << value.toString();
    return stream;
  }
  friend class chr_visitor;
	friend class tostring_visitor;
	friend class tostream_visitor;
	friend class bracket_visitor;
  friend class plus_visitor;
  friend class minus_visitor;

  typedef boost::variant< boost::blank, bool, double, str_utf8_wrapper, VectorPtr, RangeType > Variant;

private:
  static Value multvecnum(const Value &vecval, const Value &numval);
  static Value multmatvec(const VectorType &matrixvec, const VectorType &vectorvec);
  static Value multvecmat(const VectorType &vectorvec, const VectorType &matrixvec);

  Variant value;
};

void utf8_split(const std::string& str, std::function<void(Value)> f);

#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <limits>

// Workaround for https://bugreports.qt-project.org/browse/QTBUG-22829
#ifndef Q_MOC_RUN
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#endif
#include <boost/cstdint.hpp>
#include "memory.h"

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

class Value
{
public:
  class RangeType {
  private:
    double begin_val;
    double step_val;
    double end_val;
 
    /// inverse begin/end if begin is upper than end
    void normalize();

  public:
    typedef enum { RANGE_TYPE_BEGIN, RANGE_TYPE_RUNNING, RANGE_TYPE_END } type_t;
    
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
      return this->begin_val == other.begin_val &&
        this->step_val == other.step_val &&
        this->end_val == other.end_val;
    }

    double begin_value() { return begin_val; }
    double step_value() { return step_val; }
    double end_value() { return end_val; }

    iterator begin() { return iterator(*this, RANGE_TYPE_BEGIN); }
    iterator end() { return iterator(*this, RANGE_TYPE_END); }

    /// return number of steps, max uint32_t value if step is 0
    boost::uint32_t nbsteps() const;
    
    friend class chr_visitor;
    friend class tostring_visitor;
    friend class bracket_visitor;
  };

  typedef std::vector<Value> VectorType;

  enum ValueType {
    UNDEFINED,
    BOOL,
    NUMBER,
    STRING,
    VECTOR,
    RANGE
  };
  static Value undefined;

  Value();
  Value(bool v);
  Value(int v);
  Value(double v);
  Value(const std::string &v);
  Value(const char *v);
  Value(const char v);
  Value(const VectorType &v);
  Value(const RangeType &v);
  ~Value() {}

  ValueType type() const;
  bool isDefined() const;
  bool isDefinedAs(const ValueType type) const;
  bool isUndefined() const;

  double toDouble() const;
  bool getDouble(double &v) const;
  bool toBool() const;
  std::string toString() const;
  std::string chrString() const;
  const VectorType &toVector() const;
  bool getVec2(double &x, double &y) const;
  bool getVec3(double &x, double &y, double &z, double defaultval = 0.0) const;
  RangeType toRange() const;

	operator bool() const { return this->toBool(); }

  Value &operator=(const Value &v);
  bool operator==(const Value &v) const;
  bool operator!=(const Value &v) const;
  bool operator<(const Value &v) const;
  bool operator<=(const Value &v) const;
  bool operator>=(const Value &v) const;
  bool operator>(const Value &v) const;
  Value operator-() const;
  Value operator[](const Value &v) const;
  Value operator+(const Value &v) const;
  Value operator-(const Value &v) const;
  Value operator*(const Value &v) const;
  Value operator/(const Value &v) const;
  Value operator%(const Value &v) const;

  friend std::ostream &operator<<(std::ostream &stream, const Value &value) {
    if (value.type() == Value::STRING) stream << QuotedString(value.toString());
    else stream << value.toString();
    return stream;
  }

  typedef boost::variant< boost::blank, bool, double, std::string, VectorType, RangeType > Variant;

private:
  static Value multvecnum(const Value &vecval, const Value &numval);
  static Value multmatvec(const Value &matrixval, const Value &vectorval);
  static Value multvecmat(const Value &vectorval, const Value &matrixval);

  Variant value;
};

class ValuePtr : public shared_ptr<const Value>
{
public:
  static ValuePtr undefined;

	ValuePtr();
	explicit ValuePtr(const Value &v);
  ValuePtr(bool v);
  ValuePtr(int v);
  ValuePtr(double v);
  ValuePtr(const std::string &v);
  ValuePtr(const char *v);
  ValuePtr(const char v);
  ValuePtr(const Value::VectorType &v);
  ValuePtr(const Value::RangeType &v);

	operator bool() const { return **this; }

  bool operator==(const ValuePtr &v) const;
  bool operator!=(const ValuePtr &v) const;
  bool operator<(const ValuePtr &v) const;
  bool operator<=(const ValuePtr &v) const;
  bool operator>=(const ValuePtr &v) const;
  bool operator>(const ValuePtr &v) const;
  ValuePtr operator-() const;
  ValuePtr operator!() const;
  ValuePtr operator[](const ValuePtr &v) const;
  ValuePtr operator+(const ValuePtr &v) const;
  ValuePtr operator-(const ValuePtr &v) const;
  ValuePtr operator*(const ValuePtr &v) const;
  ValuePtr operator/(const ValuePtr &v) const;
  ValuePtr operator%(const ValuePtr &v) const;

  const Value &operator*() const { return *this->get(); }

private:
};

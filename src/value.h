#pragma once

#include <limits>
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <unordered_map>

// Workaround for https://bugreports.qt-project.org/browse/QTBUG-22829
#ifndef Q_MOC_RUN
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#endif

#include "AST.h"
#include "memory.h"

class tostring_visitor;
class tostream_visitor;
class ValuePtr;
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

class ValuePtr : public shared_ptr<const class Value>
{
public:
  static const ValuePtr undefined;

	ValuePtr();
	explicit ValuePtr(const Value &v);
  ValuePtr(bool v);
  ValuePtr(int v);
  ValuePtr(double v);
  ValuePtr(const std::string &v);
  ValuePtr(const char *v);
  ValuePtr(const char v);
  ValuePtr(const class std::vector<ValuePtr> &v);
  ValuePtr(const class RangeType &v);
  ValuePtr(const class ObjectType &v);
  ValuePtr(const std::shared_ptr<Expression> &v);

	operator bool() const;

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

  const Value &operator*() const;

private:
};

class ObjectType {
public:
	using map_type = std::unordered_map<std::string, ValuePtr>;
	using iterator = map_type::iterator;
	using const_iterator = map_type::const_iterator;
	using keys_type = std::vector<std::string>;
	using kiterator = keys_type::iterator;
	using const_kiterator = keys_type::const_iterator;

private:
    map_type data;
	keys_type keys;

public:
	bool has_key(const std::string& key) const;
	const keys_type& get_keys() const { return keys; };
	const ValuePtr& get(const std::string& key) const;
	void set(const std::string& key, const ValuePtr& value);

    bool operator==(const ObjectType& other) const;
};

class str_utf8_wrapper : public std::string
{
public:
	str_utf8_wrapper() : std::string(), cached_len(-1) { }
	str_utf8_wrapper( const std::string& s ) : std::string( s ), cached_len(-1) { }
	str_utf8_wrapper( size_t n, char c ) : std::string(n, c), cached_len(-1) { }
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
	typedef std::vector<ValuePtr> VectorType;

  enum class ValueType {
    UNDEFINED,
    BOOL,
    NUMBER,
    STRING,
    VECTOR,
    RANGE,
    OBJECT,
	FUNCTION
  };
  static const Value undefined;

  Value();
  Value(bool v);
  Value(int v);
  Value(double v);
  Value(const std::string &v);
  Value(const char *v);
  Value(const char v);
  Value(const VectorType &v);
  Value(const RangeType &v);
  Value(const ObjectType &v);
  Value(const std::shared_ptr<Expression> &v);
  ~Value() {}

  ValueType type() const;
  bool isDefined() const;
  bool isDefinedAs(const ValueType type) const;
  bool isUndefined() const;

  double toDouble() const;
  bool getDouble(double &v) const;
  bool getFiniteDouble(double &v) const;
  bool toBool() const;
  std::shared_ptr<Expression> toExpression() const;
  std::string typeName() const;
  std::string toString() const;
  std::string toString(const tostring_visitor *visitor) const;
  std::string toEchoString() const;
  std::string toEchoString(const tostring_visitor *visitor) const;
  void toStream(std::ostringstream &stream) const;
  void toStream(const tostream_visitor *visitor) const;
  std::string chrString() const;
  const VectorType &toVector() const;
  const ObjectType &toObject() const;
  bool getVec2(double &x, double &y, bool ignoreInfinite = false) const;
  bool getVec3(double &x, double &y, double &z) const;
  bool getVec3(double &x, double &y, double &z, double defaultval) const;
  RangeType toRange() const;

	operator bool() const { return this->toBool(); }

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
    if (value.type() == Value::ValueType::STRING) stream << QuotedString(value.toString());
    else stream << value.toString();
    return stream;
  }

  typedef boost::variant< boost::blank, bool, double, str_utf8_wrapper, VectorType, RangeType, ObjectType, std::shared_ptr<Expression>> Variant;

private:
  static Value multvecnum(const Value &vecval, const Value &numval);
  static Value multmatvec(const VectorType &matrixvec, const VectorType &vectorvec);
  static Value multvecmat(const VectorType &vectorvec, const VectorType &matrixvec);

  Variant value;
};

void utf8_split(const std::string& str, std::function<void(ValuePtr)> f);

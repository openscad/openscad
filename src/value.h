#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <limits>

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
		: begin_val(begin), step_val(1.0), end_val(end) {}
	
	RangeType(double begin, double step, double end)
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
  ValuePtr(const class FunctionType &v);

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

class FunctionType {
public:
	FunctionType(std::shared_ptr<Context> ctx, std::shared_ptr<Expression> expr, AssignmentList args)
		: ctx(ctx), expr(expr), args(args) { }
	bool operator==(const FunctionType&) const { return false; }
	bool operator!=(const FunctionType& other) const { return !(*this == other); }

	const std::shared_ptr<Context>& getCtx() { return ctx; }
	const std::shared_ptr<Expression>& getExpr() { return expr; }
	const AssignmentList& getArgs() { return args; }

	friend std::ostream& operator<<(std::ostream& stream, const FunctionType& f);

private:
	std::shared_ptr<Context> ctx;
	std::shared_ptr<Expression> expr;
	AssignmentList args;
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
  Value(const FunctionType &v);

  ValueType type() const;
  bool isDefined() const;
  bool isDefinedAs(const ValueType type) const;
  bool isUndefined() const;

  double toDouble() const;
  bool getDouble(double &v) const;
  bool getFiniteDouble(double &v) const;
  bool toBool() const;
  const FunctionType toFunction() const;
  std::string typeName() const;
  std::string toString() const;
  std::string toString(const tostring_visitor *visitor) const;
  std::string toEchoString() const;
  std::string toEchoString(const tostring_visitor *visitor) const;
  void toStream(std::ostringstream &stream) const;
  void toStream(const tostream_visitor *visitor) const;
  std::string chrString() const;
  const VectorType &toVector() const;
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

  typedef boost::variant< boost::blank, bool, double, str_utf8_wrapper, VectorType, RangeType, FunctionType> Variant;

private:
  static Value multvecnum(const Value &vecval, const Value &numval);
  static Value multmatvec(const VectorType &matrixvec, const VectorType &vectorvec);
  static Value multvecmat(const VectorType &vectorvec, const VectorType &matrixvec);

  Variant value;
};

void utf8_split(const std::string& str, std::function<void(ValuePtr)> f);

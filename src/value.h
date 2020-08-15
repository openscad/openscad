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
	static constexpr uint32_t MAX_RANGE_STEPS = 10000;

	enum class type_t { RANGE_TYPE_BEGIN, RANGE_TYPE_RUNNING, RANGE_TYPE_END };

	class iterator {
	public:
		// iterator_traits required types:
		using iterator_category = std::forward_iterator_tag ;
		using value_type        = double;
		using difference_type   = void;       // type used by operator-(iterator), not implemented for forward interator
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
	iterator end() const { return iterator(*this, type_t::RANGE_TYPE_END); }

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
  // FIXME: eventually remove this in favor of specific messages for each undef usage
  static const ValuePtr undefined;

  ValuePtr();
  explicit ValuePtr(const Value &v);
  ValuePtr(bool v);
  ValuePtr(int v);
  ValuePtr(double v);
  ValuePtr(const std::string &v);
  ValuePtr(const char *v);
  ValuePtr(const char v);
  ValuePtr(const class VectorType &v);
  ValuePtr(const class RangeType &v);
  ValuePtr(const class FunctionType &v);
  ValuePtr undef(const std::string &why); // creation of undef should provide a reason!

  operator bool() const;

  ValuePtr operator==(const ValuePtr &v) const;
  ValuePtr operator!=(const ValuePtr &v) const;
  ValuePtr operator< (const ValuePtr &v) const;
  ValuePtr operator<=(const ValuePtr &v) const;
  ValuePtr operator>=(const ValuePtr &v) const;
  ValuePtr operator> (const ValuePtr &v) const;

  ValuePtr operator-() const;
  ValuePtr operator!() const;
  ValuePtr operator[](const ValuePtr &v) const;
  ValuePtr operator+(const ValuePtr &v) const;
  ValuePtr operator-(const ValuePtr &v) const;
  ValuePtr operator*(const ValuePtr &v) const;
  ValuePtr operator/(const ValuePtr &v) const;
  ValuePtr operator%(const ValuePtr &v) const;
  ValuePtr operator^(const ValuePtr &v) const;

  const Value &operator*() const;

private:
};

class FunctionType {
public:
  FunctionType(std::shared_ptr<Context> ctx, std::shared_ptr<Expression> expr, AssignmentList args)
    : ctx(ctx), expr(expr), args(args) { }
  Value operator==(const FunctionType &other) const;
  Value operator!=(const FunctionType &other) const;
  Value operator< (const FunctionType &other) const;
  Value operator> (const FunctionType &other) const;
  Value operator<=(const FunctionType &other) const;
  Value operator>=(const FunctionType &other) const;

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

/*
  Require a reason why (string), any time an undefined value is created/returned.
  This allows for passing of "exception" information from low level functions (i.e. from value.cc)
  up to Expression::evaluate() or other functions where a proper "WARNING: ..."
  with ASTNode Location info (source file, line number) can be included.
*/
class UndefType
{
public:
  UndefType() { } // TODO: eventually deprecate undef creation without a reason.
  UndefType(const std::string &why) : reasons{why} { }

  // Append another reason in case a chain of undefined operations are made before handling
	UndefType &append(const std::string &why) { reasons.push_back(why); return *this; } 

  Value operator< (const UndefType &other) const;
  Value operator> (const UndefType &other) const;
  Value operator<=(const UndefType &other) const;
  Value operator>=(const UndefType &other) const;

  std::string toString() const;
  bool empty() const { return reasons.empty(); }
private:
  // mutable to allow clearing reasons, which should avoid duplication of warnings that have already been displayed.
  mutable std::vector<std::string> reasons;
};

class VectorType {
  using vec_t = std::vector<ValuePtr>;
public:
  using value_type = vec_t::value_type;
  using size_type = vec_t::size_type;
  using const_iterator = vec_t::const_iterator;

  VectorType() : vec() {}
  VectorType(double x, double y, double z) : vec{x,y,z} {}
  VectorType(const VectorType &) = default;
  VectorType& operator=(const VectorType &) = default;
  VectorType(VectorType&&) = default;
  VectorType& operator=(VectorType&&) = default;

  void reserve(size_type size) { vec.reserve(size); };

  Value operator==(const VectorType &v) const;
  Value operator!=(const VectorType &v) const;
  Value operator< (const VectorType &v) const;
  Value operator<=(const VectorType &v) const;
  Value operator>=(const VectorType &v) const;
  Value operator> (const VectorType &v) const;
  const ValuePtr& operator[](size_type i) const { return vec[i]; }

  const_iterator begin() const { return vec.begin(); }
  const_iterator   end() const { return vec.end();   }
  size_type size() const { return vec.size(); }
  bool empty() const { return vec.empty(); }

  void push_back(ValuePtr val) { vec.push_back(val); }
  template<typename... Args> void emplace_back(Args&&... args) { vec.emplace_back(std::forward<Args>(args)...); }

private:
  vec_t vec;
};

class Value
{
public:

  enum class Type {
    UNDEFINED,
    BOOL,
    NUMBER,
    STRING,
    VECTOR,
    RANGE,
    FUNCTION
  };
  // FIXME: eventually remove this in favor of specific messages for each undef usage
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
  Value(const UndefType &v);

  static Value undef(const std::string &why); // creation of undef requires a reason!

  Type type() const;
  bool isDefined() const;
  bool isDefinedAs(const Type type) const;
  bool isUndefined() const;
  bool isUncheckedUndef() const;

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
  UndefType& toUndef();
  std::string toUndefString() const;
  void toStream(std::ostringstream &stream) const;
  void toStream(const tostream_visitor *visitor) const;
  std::string chrString() const;
  const VectorType &toVector() const;
  bool getVec2(double &x, double &y, bool ignoreInfinite = false) const;
  bool getVec3(double &x, double &y, double &z) const;
  bool getVec3(double &x, double &y, double &z, double defaultval) const;
  RangeType toRange() const;

  operator bool() const { return this->toBool(); }

  Value operator==(const Value &v) const;
  Value operator!=(const Value &v) const;
  Value operator< (const Value &v) const;
  Value operator<=(const Value &v) const;
  Value operator>=(const Value &v) const;
  Value operator> (const Value &v) const;
  Value operator-() const;
  Value operator[](const Value &v) const;
  Value operator+(const Value &v) const;
  Value operator-(const Value &v) const;
  Value operator*(const Value &v) const;
  Value operator/(const Value &v) const;
  Value operator%(const Value &v) const;
  Value operator^(const Value &v) const;

  friend std::ostream &operator<<(std::ostream &stream, const Value &value) {
    if (value.type() == Value::Type::STRING) stream << QuotedString(value.toString());
    else stream << value.toString();
    return stream;
  }

  typedef boost::variant<UndefType, bool, double, str_utf8_wrapper, VectorType, RangeType, FunctionType> Variant;

private:
  Variant value;
};

void utf8_split(const std::string& str, std::function<void(ValuePtr)> f);

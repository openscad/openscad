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

template <typename T>
class ValuePtr {
private:
	explicit ValuePtr(const std::shared_ptr<T> &val_in) : value(val_in) { }
public:
	ValuePtr(T&& value) : value(std::make_shared<T>(std::move(value))) { }
	ValuePtr clone() const { return ValuePtr(value); }

	const T& operator*() const { return *value; }
	const T* operator->() const { return value.get(); }
	bool operator==(const ValuePtr& other) const { return *value == *other; }
	bool operator!=(const ValuePtr& other) const { return !(*this == other); }
	bool operator< (const ValuePtr& other) const { return *value <  *other; }
	bool operator> (const ValuePtr& other) const { return *value >  *other; }
	bool operator<=(const ValuePtr& other) const { return *value <= *other; }
	bool operator>=(const ValuePtr& other) const { return *value >= *other; }

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
	FunctionType(std::shared_ptr<Context> ctx, std::shared_ptr<Expression> expr, AssignmentList args)
		: ctx(ctx), expr(expr), args(args) { }
	bool operator==(const FunctionType& other) const { return this == &other; }
	bool operator!=(const FunctionType& other) const { return this != &other; }
	bool operator< (const FunctionType&) const { return false; }
	bool operator> (const FunctionType&) const { return false; }
	bool operator<=(const FunctionType&) const { return false; }
	bool operator>=(const FunctionType&) const { return false; }

	const std::shared_ptr<Context>& getCtx() const { return ctx; }
	const std::shared_ptr<Expression>& getExpr() const { return expr; }
	const AssignmentList& getArgs() const { return args; }

private:
	std::shared_ptr<Context> ctx;
	std::shared_ptr<Expression> expr;
	AssignmentList args;
};

using FunctionPtr = ValuePtr<FunctionType>;

std::ostream& operator<<(std::ostream& stream, const FunctionType& f);


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
  static const Value undefined;
	using VectorType = std::vector<Value>;

  class VectorPtr {
  public:
    VectorPtr() : ptr(new VectorType()) {}
		VectorPtr(const VectorPtr &) = delete;            // never copy, move instead
		VectorPtr& operator=(const VectorPtr &) = delete; // never copy, move instead
		VectorPtr(VectorPtr&&) = default;
		VectorPtr& operator=(VectorPtr&&) = default;
    
    // Copy explicitly only when necessary
    VectorPtr clone() const { VectorPtr c; c.ptr = this->ptr; return c;  }

    const VectorType &operator*() const { return *ptr.get(); }
    VectorType *operator->() const { return ptr.get(); }
    Value &operator[](size_t idx) const {	return (*ptr.get())[idx]; }
    bool operator==(const VectorPtr &v) const { return *ptr == *v; }
    bool operator!=(const VectorPtr &v) const {	return *ptr != *v; }
    operator bool() const {	return !ptr->empty(); }
   
  private:
    shared_ptr<VectorType> ptr;
  };

  Value() : value(boost::blank()) { }
	static Value undef() { return Value(); }
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

  Type type() const;
  bool isDefined() const;
  bool isDefinedAs(const Type type) const;
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
  const VectorPtr &toVectorPtr() const;
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
    if (value.type() == Value::Type::STRING) stream << QuotedString(value.toString());
    else stream << value.toString();
    return stream;
  }

  typedef boost::variant< boost::blank, bool, double, str_utf8_wrapper, VectorPtr, RangeType, FunctionPtr> Variant;

private:
  static Value multvecnum(const Value &vecval, const Value &numval);
  static Value multmatvec(const VectorType &matrixvec, const VectorType &vectorvec);
  static Value multvecmat(const VectorType &vectorvec, const VectorType &matrixvec);

  Variant value;
};

void utf8_split(const std::string& str, std::function<void(Value)> f);
using VectorType = Value::VectorType;

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
		iterator(const RangeType &range, type_t type);
		self_type operator++();
		self_type operator++(int junk);
		reference operator*();
		pointer operator->();
		bool operator==(const self_type& other) const;
		bool operator!=(const self_type& other) const;
	private:
		const RangeType &range;
		double val;
		type_t type;
		
		void update_type();
	};
	
	RangeType(const RangeType &) = delete; // never copy, move instead
	RangeType& operator=(const RangeType &) = delete; // never copy, move instead
	RangeType(RangeType&&) = default; // default move constructor
	RangeType& operator=(RangeType&&) = default; // default move assignment 
	
	// Copy explicitly only when necessary
	RangeType clone() const { return RangeType(this->begin_val,this->step_val,this->end_val); }; 

	explicit RangeType(double begin, double end)
		: begin_val(begin), step_val(1.0), end_val(end)
		{
			normalize();
		}
	
	explicit RangeType(double begin, double step, double end)
		: begin_val(begin), step_val(step), end_val(end) {}
	
	bool operator==(const RangeType &other) const {
		return this == &other ||
			(this->begin_val == other.begin_val &&
			 this->step_val == other.step_val &&
			 this->end_val == other.end_val);
	}
	
	double begin_value() const { return begin_val; }
	double step_value() const { return step_val; }
	double end_value() const { return end_val; }
	
	iterator begin() const { return iterator(*this, type_t::RANGE_TYPE_BEGIN); }
	iterator end() const{ return iterator(*this, type_t::RANGE_TYPE_END); }
	
	/// return number of values, max uint32_t value if step is 0 or range is infinite
	uint32_t numValues() const;
	
	friend class chr_visitor;
	friend class tostring_visitor;
	friend class tostream_visitor;
	friend class bracket_visitor;
};

class str_utf8_wrapper
{
	// store the cached length in glong, paired with its string
	typedef std::pair<std::string, glong> str_utf8_t;
	typedef shared_ptr<str_utf8_t> str_ptr_t;
	// private constructor for copying members
	explicit str_utf8_wrapper(str_ptr_t str_in) : str_ptr(str_in) { }
public:
	str_utf8_wrapper() : str_ptr(new str_utf8_t{ std::string{""}, -1}) { }
	explicit str_utf8_wrapper( const std::string& s ) : str_ptr(new str_utf8_t{std::string{s}, -1}) { }
	explicit str_utf8_wrapper( size_t n, char c ) : str_ptr(new str_utf8_t{std::string(n, c),-1}) { }

	str_utf8_wrapper(const str_utf8_wrapper &) = delete; // never copy, move instead
	str_utf8_wrapper& operator=(const str_utf8_wrapper &) = delete; // never copy, move instead
	str_utf8_wrapper(str_utf8_wrapper&&) = default; // default move constructor
	str_utf8_wrapper& operator=(str_utf8_wrapper&&) = default; // default move assignment 

	// makes a copy of shared_ptr
	str_utf8_wrapper clone() const { return str_utf8_wrapper(this->str_ptr); }

	bool operator==(const str_utf8_wrapper &rhs) const { return this->str_ptr->first == rhs.str_ptr->first; }
	bool empty() const { return this->str_ptr->first.empty(); }
	const char* c_str() const noexcept { return this->str_ptr->first.c_str(); }
	size_t size() const noexcept { return this->str_ptr->first.size(); }

	glong get_utf8_strlen() const {
		if (str_ptr->second < 0) {
			str_ptr->second = g_utf8_strlen(this->str_ptr->first.c_str(), this->str_ptr->first.size());
		}
		return str_ptr->second;
	};

	friend class tostring_visitor;
	friend class tostream_visitor;
	friend class less_visitor;
	friend class greater_visitor;
	friend class lessequal_visitor;
	friend class greaterequal_visitor;

private:
	str_ptr_t str_ptr;
};

class Value
{
public:
	typedef std::vector<Value> VectorType;

	class VectorPtr {
	public:
		VectorPtr() : ptr(new VectorType()) {}
		VectorPtr(const VectorPtr &) = delete; // never copy, move instead
		VectorPtr& operator=(const VectorPtr &) = delete; // never copy, move instead
		VectorPtr(VectorPtr&&) = default; // default move constructor
		VectorPtr& operator=(VectorPtr&&) = default; // default move assignment 

		// Copy explicitly only when necessary
		VectorPtr clone() const { VectorPtr c; c.ptr = this->ptr; return c;  }

		const Value::VectorType &operator*() const { return *ptr; }
		Value::VectorType *operator->() const { return ptr.get(); }
		Value &operator[](size_t idx) const {
			static Value undef;
			return idx < ptr->size() ? (*ptr)[idx] : undef; 
		}
		bool operator==(const VectorPtr &v) const { return *ptr == *v; }
		bool operator!=(const VectorPtr &v) const {	return *ptr != *v; }
		bool empty() const { return ptr->empty(); }
		
		friend bool operator==(const VectorPtr &vptr, nullptr_t) { vptr.ptr == nullptr; };
		friend bool operator!=(const VectorPtr &vptr, nullptr_t) { vptr.ptr != nullptr; };
		friend bool operator==(nullptr_t, const VectorPtr &vptr) { nullptr == vptr.ptr; };
		friend bool operator!=(nullptr_t, const VectorPtr &vptr) { nullptr != vptr.ptr; };

	private:
		shared_ptr<Value::VectorType> ptr;
	};

	enum class ValueType {
		UNDEFINED,
		BOOL,
		NUMBER,
		STRING,
		VECTOR,
		RANGE
	};
	static const Value undefined;

	Value();
	Value(const Value &) = delete; // never copy, move instead
	Value &operator=(const Value &v) = delete; // never copy, move instead
	Value(Value&&) = default; // default move constructor
	Value& operator=(Value&&) = default; // default move assignment
	explicit Value(bool v);
	explicit Value(int v);
	explicit Value(double v);
	explicit Value(const std::string &v);
	explicit Value(const char *v);
	explicit Value(const char v);
	explicit Value(RangeType&& v) : value(std::move(v)) {};
	explicit Value(VectorPtr&& v) : value(std::move(v)) {};

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
	std::string toString(const tostring_visitor *visitor) const;
	std::string toEchoString() const;
	std::string toEchoString(const tostring_visitor *visitor) const;
	const str_utf8_wrapper& toStrUtf8Wrapper();
	void toStream(std::ostringstream &stream) const;
	void toStream(const tostream_visitor *visitor) const;
	std::string chrString() const;
	const VectorPtr &toVectorPtr() const;

	bool getVec2(double &x, double &y, bool ignoreInfinite = false) const;
	bool getVec3(double &x, double &y, double &z) const;
	bool getVec3(double &x, double &y, double &z, double defaultval) const;
	const RangeType& toRange() const;

	bool operator==(const Value &v) const;
	bool operator!=(const Value &v) const;
	bool operator<(const Value &v) const;
	bool operator<=(const Value &v) const;
	bool operator>=(const Value &v) const;
	bool operator>(const Value &v) const;
	Value operator-() const;
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

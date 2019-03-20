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
private:
	// store the cached length in glong, paired with its string
	typedef std::pair<std::string, glong> str_utf8_t;
	typedef shared_ptr<str_utf8_t> str_ptr_t;
	// private constructor for copying members
	explicit str_utf8_wrapper(str_ptr_t str_in) : str_ptr(str_in) { }
public:
	str_utf8_wrapper() : str_ptr(make_shared<str_utf8_t>(str_utf8_t{"", -1})) { }
	explicit str_utf8_wrapper( const std::string& s ) : str_ptr(make_shared<str_utf8_t>(s, -1)) { }
	explicit str_utf8_wrapper( size_t n, char c ) : str_ptr(make_shared<str_utf8_t>(std::string(n, c) ,-1)) { }

	str_utf8_wrapper(const str_utf8_wrapper &) = delete; // never copy, move instead
	str_utf8_wrapper& operator=(const str_utf8_wrapper &) = delete; // never copy, move instead
	str_utf8_wrapper(str_utf8_wrapper&&) = default; // default move constructor
	str_utf8_wrapper& operator=(str_utf8_wrapper&&) = default; // default move assignment 

	// makes a copy of shared_ptr
	str_utf8_wrapper clone() const noexcept { return str_utf8_wrapper(this->str_ptr); }

	bool operator==(const str_utf8_wrapper &rhs) const noexcept { return this->str_ptr->first == rhs.str_ptr->first; }
	bool operator< (const str_utf8_wrapper &rhs) const noexcept { return this->str_ptr->first <  rhs.str_ptr->first; }
	bool operator> (const str_utf8_wrapper &rhs) const noexcept { return this->str_ptr->first >  rhs.str_ptr->first; }
	bool operator<=(const str_utf8_wrapper &rhs) const noexcept { return this->str_ptr->first <= rhs.str_ptr->first; }
	bool operator>=(const str_utf8_wrapper &rhs) const noexcept { return this->str_ptr->first >= rhs.str_ptr->first; }
	bool empty() const noexcept { return this->str_ptr->first.empty(); }
	const char* c_str() const noexcept { return this->str_ptr->first.c_str(); }
	const std::string& toString() const noexcept { return this->str_ptr->first; } 
	size_t size() const noexcept { return this->str_ptr->first.size(); }

	glong get_utf8_strlen() const {
		if (str_ptr->second < 0) {
			str_ptr->second = g_utf8_strlen(this->str_ptr->first.c_str(), this->str_ptr->first.size());
		}
		return str_ptr->second;
	};


private:
	str_ptr_t str_ptr;
};

class Value
{
public:
	typedef std::vector<Value> VectorType;
	enum class ValueType {
		UNDEFINED,
		BOOL,
		NUMBER,
		STRING,
		VECTOR,
		RANGE
	};
	static const Value undefined;

	class VectorPtr {
	public:
		VectorPtr() : ptr(make_shared<VectorType>()) {}
		VectorPtr(double x, double y, double z);
		VectorPtr(const VectorPtr &) = delete; // never copy, move instead
		VectorPtr& operator=(const VectorPtr &) = delete; // never copy, move instead
		VectorPtr(VectorPtr&&) = default; // default move constructor
		VectorPtr& operator=(VectorPtr&&) = default; // default move assignment 

		// Copy explicitly only when necessary
		// We can't use unique_ptr because of this :(
		VectorPtr clone() const { VectorPtr c; c.ptr = this->ptr; return c; }

		const Value::VectorType &operator*() const noexcept { return *ptr; }
		Value::VectorType *operator->() const noexcept { return ptr.get(); }
		// const accesses to VectorType require .clone to be move-able
		const Value &operator[](size_t idx) const noexcept { return idx < ptr->size() ? (*ptr)[idx] : Value::undefined; }
		// non-const return operator[] is only accessible from protected toVectorPtrRef
		Value &operator[](size_t idx) noexcept {
			static Value undef;
			return idx < ptr->size() ? (*ptr)[idx] : undef; 
		}
		bool operator==(const VectorPtr &v) const { return *ptr == *v; }
		bool operator!=(const VectorPtr &v) const {	return *ptr != *v; }
		
		void flatten();

	private:
		shared_ptr<Value::VectorType> ptr;
	};

	Value() : value(boost::blank()) {}
	Value(const Value &) = delete; // never copy, move instead
	Value &operator=(const Value &v) = delete; // never copy, move instead
	Value(Value&&) = default; // default move constructor
	Value& operator=(Value&&) = default; // default move assignment
	explicit Value(bool v) : value(v) {}
	explicit Value(int v) : value(double(v)) {}
	explicit Value(double v) : value(v) {}
	explicit Value(const std::string &v) : value(str_utf8_wrapper(v)) {}
	explicit Value(const char *v) : value(str_utf8_wrapper(v)) {}
	explicit Value(const char v) : value(str_utf8_wrapper(1, v)) {}
	explicit Value(RangeType& v) : value(std::move(v)) {}
	explicit Value(RangeType&& v) : value(std::move(v)) {}
	explicit Value(VectorPtr& v) : value(std::move(v)) {}
	explicit Value(VectorPtr&& v) : value(std::move(v)) {}

	Value clone() const; // Use sparingly to explicitly copy a Value

	ValueType type() const { return static_cast<ValueType>(this->value.which()); }
	bool isDefinedAs(const ValueType type) const { return this->type() == type; }
	bool isDefined()   const { return this->type() != ValueType::UNDEFINED; }
	bool isUndefined() const { return this->type() == ValueType::UNDEFINED; }

	double toDouble() const;
	bool getDouble(double &v) const;
	bool getFiniteDouble(double &v) const;
	bool toBool() const;
	std::string toString() const;
	std::string toString(const tostring_visitor *visitor) const;
	std::string toEchoString() const;
	std::string toEchoString(const tostring_visitor *visitor) const;
	const str_utf8_wrapper& toStrUtf8Wrapper() const;
	void toStream(std::ostringstream &stream) const;
	void toStream(const tostream_visitor *visitor) const;
	std::string chrString() const;
	const VectorPtr &toVectorPtr() const;
protected:
	// unsafe non-const reference needed by VectorPtr::flatten
	VectorPtr &toVectorPtrRef() { return boost::get<VectorPtr>(this->value); };
public:
	bool getVec2(double &x, double &y, bool ignoreInfinite = false) const;
	bool getVec3(double &x, double &y, double &z) const;
	bool getVec3(double &x, double &y, double &z, double defaultval) const;
	const RangeType& toRange() const;

	operator bool() const { return this->toBool(); }
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

	typedef boost::variant< boost::blank, bool, double, str_utf8_wrapper, VectorPtr, RangeType > Variant;

private:
	static Value multvecnum(const Value &vecval, const Value &numval);
	static Value multmatvec(const VectorType &matrixvec, const VectorType &vectorvec);
	static Value multvecmat(const VectorType &vectorvec, const VectorType &matrixvec);

	Variant value;
};

void utf8_split(const str_utf8_wrapper& str, std::function<void(Value)> f);

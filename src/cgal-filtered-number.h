// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
//
// Based on my related "fuzzy number" attempt posted here: https://github.com/CGAL/cgal/issues/5490
//
#pragma once

#include <cmath>
#include <csignal>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <boost/variant.hpp>

#ifndef LAZY_FILTERED_NUMBER
#define LAZY_FILTERED_NUMBER 0
#endif

#define FILTERED_NUMBER_OP_TEMPLATE(T)                                                           \
	template <typename T,                                                                          \
						std::enable_if_t<(std::is_arithmetic<T>::value || std::is_assignable<FT, T>::value), \
														 bool> = true>

#ifndef FILTERED_NUMBER_INTERVAL_TYPE
#define FILTERED_NUMBER_INTERVAL_TYPE double
#endif

// const FILTERED_NUMBER_INTERVAL_TYPE EPSILON = 2e-20;
const FILTERED_NUMBER_INTERVAL_TYPE EPSILON = 0.00001;

template <class FT>
class FilteredNumber
{
	typedef FILTERED_NUMBER_INTERVAL_TYPE BoundsType;
	typedef FilteredNumber<FT> Type;
	typedef std::function<FT()> ValueGetter;

#if LAZY_FILTERED_NUMBER
	mutable boost::variant<FT, ValueGetter> value_;
#else
	mutable FT value_;
#endif
	BoundsType lower_, upper_;

	enum IntervalComparison { SMALLER, LARGER, WITHIN_INTERVAL };

	IntervalComparison compareTo(const Type &other) const
	{
		if (upper_ < other.lower_) {
			return IntervalComparison::SMALLER;
		}
		if (lower_ > other.upper_) {
			return IntervalComparison::LARGER;
		}
		return IntervalComparison::WITHIN_INTERVAL;
	}

	FILTERED_NUMBER_OP_TEMPLATE(T) IntervalComparison compareTo(const T &x) const
	{
		if (upper_ < x) {
			return IntervalComparison::SMALLER;
		}
		if (lower_ > x) {
			return IntervalComparison::LARGER;
		}
		return IntervalComparison::WITHIN_INTERVAL;
	}

	FilteredNumber(
#if LAZY_FILTERED_NUMBER
			const boost::variant<FT, ValueGetter> &value,
#else
			const FT &value,
#endif
			BoundsType lower, BoundsType upper)
		: value_(value), lower_(lower), upper_(upper)
	{
		assert(lower_ < upper_);
		assert(lower_ < exact());
		assert(upper_ > exact());
	}

	// The bool is just here to avoid any implicit usage & conflict with other ctors.
	FilteredNumber(const FT &value, BoundsType doubleValue, BoundsType delta, bool)
		: FilteredNumber(value, doubleValue - delta, doubleValue + delta)
	{
	}

public:
	FilteredNumber(double value)
		: FilteredNumber(FT(value), static_cast<BoundsType>(value), EPSILON, false)
	{
	}
	FilteredNumber(const FT &value)
		: FilteredNumber(value, static_cast<BoundsType>(CGAL::to_double(value)), EPSILON, false)
	{
	}

	template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
	FilteredNumber(T value)
		: FilteredNumber(FT(value), static_cast<BoundsType>(value), EPSILON, false)
	{
	}
	// FilteredNumber(const T& value) : FilteredNumber(static_cast<double>(value)) {} //
	// static_cast<double>(value)?

	FilteredNumber(const Type &other) : FilteredNumber(other.value_, other.lower_, other.upper_) {}
	FilteredNumber() : FilteredNumber(0.0) {}

	const FT &exact() const
	{
#if LAZY_FILTERED_NUMBER
		if (auto pValue = boost::get<FT>(&value_)) {
			return *pValue;
		}
		if (auto pGetter = boost::get<ValueGetter>(&value_)) {
			value_ = (*pGetter)();
			// TODO(ochafik): once C++17 allowed, return value_.emplace((*pGetter)());
			return *boost::get<FT>(&value_);
		}
		throw 0;
#else
		return value_;
#endif
	}
	explicit operator double() const { return CGAL::to_double(exact()); }
	std::pair<double, double> interval() const { return std::make_pair(lower_, upper_); }

	Type operator-() const
	{
		auto a = exact();
		return Type(
#if LAZY_FILTERED_NUMBER
				[=]() { return -a; },
#else
				-a,
#endif
				-upper_, -lower_);
	}

	Type sqrt() const
	{
		auto a = exact();
		// Are two double sqrt faster than a single exact one?
		return Type(
#if LAZY_FILTERED_NUMBER
				[=]() { return std::sqrt(a); },
#else
				std::sqrt(a),
#endif
				std::max(0, std::sqrt(std::max(lower_, 0)) - EPSILON), std::sqrt(upper_) + EPSILON);
	}

	bool operator==(const Type &other) const
	{
		if (compareTo(other) != IntervalComparison::WITHIN_INTERVAL) {
			return false;
		}
		return exact() == other.exact();
	}

	FILTERED_NUMBER_OP_TEMPLATE(T) bool operator==(const T &x) const
	{
		if (compareTo(x) != IntervalComparison::WITHIN_INTERVAL) {
			return false;
		}
		return exact() == x;
	}

	bool operator<(const Type &other) const
	{
		switch (compareTo(other)) {
		case SMALLER:
			return true;
		case LARGER:
			return false;
		default:
			return exact() < other.exact();
		}
	}
	FILTERED_NUMBER_OP_TEMPLATE(T) bool operator<(const T &x) const
	{
		switch (compareTo(x)) {
		case SMALLER:
			return true;
		case LARGER:
			return false;
		default:
			return exact() < x;
		}
	}

	// a >= b if !(a < b)
	bool operator>=(const Type &other) const { return !(*this < other); }
	FILTERED_NUMBER_OP_TEMPLATE(T) bool operator>=(const T &x) const { return !(*this < x); }

	bool operator>(const Type &other) const
	{
		switch (compareTo(other)) {
		case SMALLER:
			return false;
		case LARGER:
			return true;
		default:
			return exact() > other.exact();
		}
	}
	FILTERED_NUMBER_OP_TEMPLATE(T) bool operator>(const T &x) const
	{
		switch (compareTo(x)) {
		case SMALLER:
			return false;
		case LARGER:
			return true;
		default:
			return exact() > x;
		}
	}

	// a <= b if !(a > b)
	bool operator<=(const Type &other) const { return !(*this > other); }
	FILTERED_NUMBER_OP_TEMPLATE(T) bool operator<=(const T &x) const { return !(*this > x); }

	bool operator!=(const Type &other) const { return !(*this == other); }
	FILTERED_NUMBER_OP_TEMPLATE(T) bool operator!=(const T &x) const
	{
		if (compareTo(x) != IntervalComparison::WITHIN_INTERVAL) {
			return true;
		}
		return exact() != x;
	}

	Type min(const Type &other) const
	{
		switch (compareTo(other)) {
		case SMALLER:
			return *this;
		case LARGER:
			return other;
		default:
			return exact() < other.exact() ? *this : other;
		}
	}

	Type max(const Type &other) const
	{
		switch (compareTo(other)) {
		case SMALLER:
			return other;
		case LARGER:
			return *this;
		default:
			return exact() > other.exact() ? *this : other;
		}
	}

	Type operator+(const Type &other) const
	{
		auto lhs = exact(), rhs = other.exact();
		return Type(
#if LAZY_FILTERED_NUMBER
				[=]() { return lhs + rhs; },
#else
				lhs + rhs,
#endif
				lower_ + other.lower_, upper_ + other.upper_);
	}
	FILTERED_NUMBER_OP_TEMPLATE(T) FilteredNumber<FT> operator+(const T &x) const
	{
		if (x == 0) {
			return *this;
		}
		return *this + Type(x);
	}
	FilteredNumber<FT> &operator+=(const Type &x) { return *this = *this + x; }
	FILTERED_NUMBER_OP_TEMPLATE(T) FilteredNumber<FT> &operator+=(const T &x)
	{
		return *this = *this + x;
	}

	Type operator-(const Type &other) const
	{
		auto lhs = exact(), rhs = other.exact();
		return Type(
#if LAZY_FILTERED_NUMBER
				[=]() { return lhs - rhs; },
#else
				lhs - rhs,
#endif
				lower_ - other.upper_, upper_ - other.lower_);
	}
	FILTERED_NUMBER_OP_TEMPLATE(T) FilteredNumber<FT> operator-(const T &x) const
	{
		if (x == 0) {
			return *this;
		}
		return *this - Type(x);
	}
	FilteredNumber<FT> &operator-=(const Type &x) { return *this = *this - x; }
	FILTERED_NUMBER_OP_TEMPLATE(T) FilteredNumber<FT> &operator-=(const T &x)
	{
		return *this = *this - x;
	}

	Type operator*(const Type &other) const
	{
		auto lhs = exact(), rhs = other.exact();
		auto ll = lower_ * other.lower_;
		auto uu = upper_ * other.upper_;

		double newLower, newUpper;
		if (lower_ >= 0 && other.lower_ >= 0) {
			newLower = ll;
			newUpper = uu;
		}
		else {
			auto lu = lower_ * other.upper_;
			auto ul = upper_ * other.lower_;
			newLower = std::min({ll, uu, lu, ul});
			newUpper = std::max({ll, uu, lu, ul});
		}

		return Type(
#if LAZY_FILTERED_NUMBER
				[=]() { return lhs * rhs; },
#else
				lhs * rhs,
#endif
				newLower, newUpper);
	}
	FILTERED_NUMBER_OP_TEMPLATE(T) FilteredNumber<FT> operator*(const T &x) const
	{
		if (x == 0) {
			return Type(0);
		}
		return *this * Type(x);
	}
	FilteredNumber<FT> &operator*=(const Type &x) { return *this = *this * x; }
	FILTERED_NUMBER_OP_TEMPLATE(T) FilteredNumber<FT> &operator*=(const T &x)
	{
		return *this = *this * x;
	}

	Type operator/(const Type &other) const
	{
		// if (other.lower_ == 0 && other.upper_ == 0) {
		//   raise(SIGFPE); // Throw a floating point exception.
		//   throw 0; // We won't reach this point.
		// }
		auto lhs = exact(), rhs = other.exact();
		// TODO(ochafik): division interval arithmetics
		// (https://en.wikipedia.org/wiki/Interval_arithmetic).
		return Type(lhs / rhs);
	}
	FILTERED_NUMBER_OP_TEMPLATE(T) FilteredNumber<FT> operator/(const T &x) const
	{
		return *this / Type(x);
	}
	FilteredNumber<FT> &operator/=(const Type &x) { return *this = *this / x; }
	FILTERED_NUMBER_OP_TEMPLATE(T) FilteredNumber<FT> &operator/=(const T &x)
	{
		return *this = *this / x;
	}
};

#define FILTERED_NUMBER_BINARY_NUMERIC_OP_FN(functionName, op)                              \
	template <class T, class FT, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true> \
	FilteredNumber<FT> functionName(const T &lhs, const FilteredNumber<FT> &rhs)              \
	{                                                                                         \
		return FilteredNumber<FT>(lhs) op rhs;                                                  \
	}

#define FILTERED_NUMBER_BINARY_BOOL_OP_FN(functionName, op)                                 \
	template <class T, class FT, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true> \
	bool functionName(const T &lhs, const FilteredNumber<FT> &rhs)                            \
	{                                                                                         \
		return FilteredNumber<FT>(lhs) op rhs;                                                  \
	}

FILTERED_NUMBER_BINARY_NUMERIC_OP_FN(operator+, +)
FILTERED_NUMBER_BINARY_NUMERIC_OP_FN(operator-, -)
FILTERED_NUMBER_BINARY_NUMERIC_OP_FN(operator*, *)
FILTERED_NUMBER_BINARY_NUMERIC_OP_FN(operator/, /)

FILTERED_NUMBER_BINARY_BOOL_OP_FN(operator<, <)
FILTERED_NUMBER_BINARY_BOOL_OP_FN(operator>, >)
FILTERED_NUMBER_BINARY_BOOL_OP_FN(operator<=, <=)
FILTERED_NUMBER_BINARY_BOOL_OP_FN(operator>=, >=)

template <class FT>
std::ostream &operator<<(std::ostream &out, const FilteredNumber<FT> &n)
{
	out << n.exact();
	return out;
}
template <class FT>
std::istream &operator>>(std::istream &in, FilteredNumber<FT> &n)
{
	in >> n.exact();
	return in;
}

namespace std {
template <class FT>
FilteredNumber<FT> sqrt(const FilteredNumber<FT> &x)
{
	return x.sqrt();
}
} // namespace std

namespace CGAL {
template <class FT>
inline FilteredNumber<FT> min BOOST_PREVENT_MACRO_SUBSTITUTION(const FilteredNumber<FT> &x,
																															 const FilteredNumber<FT> &y)
{
	return x.min(y);
}
template <class FT>
inline FilteredNumber<FT> max BOOST_PREVENT_MACRO_SUBSTITUTION(const FilteredNumber<FT> &x,
																															 const FilteredNumber<FT> &y)
{
	return x.max(y);
}

CGAL_DEFINE_COERCION_TRAITS_FOR_SELF(FilteredNumber<CGAL::Gmpq>)
CGAL_DEFINE_COERCION_TRAITS_FROM_TO(short, FilteredNumber<CGAL::Gmpq>)
CGAL_DEFINE_COERCION_TRAITS_FROM_TO(int, FilteredNumber<CGAL::Gmpq>)
CGAL_DEFINE_COERCION_TRAITS_FROM_TO(long, FilteredNumber<CGAL::Gmpq>)
CGAL_DEFINE_COERCION_TRAITS_FROM_TO(float, FilteredNumber<CGAL::Gmpq>)
CGAL_DEFINE_COERCION_TRAITS_FROM_TO(double, FilteredNumber<CGAL::Gmpq>)

template <class FT>
class Algebraic_structure_traits<FilteredNumber<FT>>
	: public Algebraic_structure_traits_base<FilteredNumber<FT>, Field_with_sqrt_tag>
{
	typedef Algebraic_structure_traits<FT> Underlying_traits;

public:
	typedef typename Underlying_traits::Is_exact Is_exact;
	typedef typename Underlying_traits::Is_numerical_sensitive Is_numerical_sensitive;
};

template <class FT>
class Real_embeddable_traits<FilteredNumber<FT>>
	: public INTERN_RET::Real_embeddable_traits_base<FilteredNumber<FT>, CGAL::Tag_true>
{
	typedef Real_embeddable_traits<FT> Underlying_traits;
	typedef FilteredNumber<FT> Type;

public:
	typedef typename Underlying_traits::Is_real_embeddable Is_real_embeddable;
	typedef typename Underlying_traits::Boolean Boolean;
	typedef typename Underlying_traits::Comparison_result Comparison_result;
	typedef typename Underlying_traits::Sign Sign;
	typedef typename Underlying_traits::Is_zero Is_zero;

	class Is_positive : public CGAL::cpp98::unary_function<Type, bool>
	{
	public:
		bool operator()(const Type &x_) const { return x_ > 0; }
	};

	class Is_negative : public CGAL::cpp98::unary_function<Type, bool>
	{
	public:
		bool operator()(const Type &x_) const { return x_ < 0; }
	};

	struct Is_finite : public CGAL::cpp98::unary_function<Type, Boolean> {
		inline Boolean operator()(const Type &x) const
		{
			return Underlying_traits::Is_finite()(x.exact());
		}
	};

	struct To_interval : public CGAL::cpp98::unary_function<Type, std::pair<double, double>> {
		inline std::pair<double, double> operator()(const Type &x) const
		{
			typename Underlying_traits::To_interval to_interval;

			return x.interval();
		}
	};
};

namespace internal {

template <class T>
struct Exact_field_selector<FilteredNumber<T>> {
	typedef T Type;
};

} // namespace internal

template <>
class Fraction_traits<FilteredNumber<CGAL::Gmpq>>
{
public:
	typedef FilteredNumber<CGAL::Gmpq> Type;
	typedef ::CGAL::Tag_true Is_fraction;
	typedef Gmpz Numerator_type;
	typedef Gmpz Denominator_type;
	typedef Algebraic_structure_traits<Gmpz>::Gcd Common_factor;
	class Decompose
	{
	public:
		typedef FilteredNumber<CGAL::Gmpq> first_argument_type;
		typedef Gmpz &second_argument_type;
		typedef Gmpz &third_argument_type;
		void operator()(const FilteredNumber<CGAL::Gmpq> &rat, Gmpz &num, Gmpz &den)
		{
			// TODO(ochafik): compose proper singleton num and denom, and their transition to a singleton
			// rational.
			num = rat.exact().numerator();
			den = rat.exact().denominator();
		}
	};
	class Compose
	{
	public:
		typedef Gmpz first_argument_type;
		typedef Gmpz second_argument_type;
		typedef FilteredNumber<CGAL::Gmpq> result_type;
		FilteredNumber<CGAL::Gmpq> operator()(const Gmpz &num, const Gmpz &den)
		{
			// TODO(ochafik): compose proper singleton num and denom, and their transition to a singleton
			// rational.
			return FilteredNumber<CGAL::Gmpq>(CGAL::Gmpq(num, den));
		}
	};
};

class FilteredNumberGMP_arithmetic_kernel : public internal::Arithmetic_kernel_base
{
public:
	typedef Gmpz Integer;
	typedef FilteredNumber<CGAL::Gmpq> Rational;
};

template <>
struct Get_arithmetic_kernel<FilteredNumber<Gmpz>> {
	typedef FilteredNumberGMP_arithmetic_kernel Arithmetic_kernel;
};
template <>
struct Get_arithmetic_kernel<FilteredNumber<Gmpq>> {
	typedef FilteredNumberGMP_arithmetic_kernel Arithmetic_kernel;
};
} // namespace CGAL

namespace Eigen {
template <class>
struct NumTraits;
template <>
struct NumTraits<FilteredNumber<CGAL::Gmpq>> {
	typedef FilteredNumber<CGAL::Gmpq> Real;
	typedef FilteredNumber<CGAL::Gmpq> NonInteger;
	typedef FilteredNumber<CGAL::Gmpq> Nested;
	typedef FilteredNumber<CGAL::Gmpq> Literal;

	static inline Real epsilon() { return 0; }
	static inline Real dummy_precision() { return 0; }

	enum {
		IsInteger = 0,
		IsSigned = 1,
		IsComplex = 0,
		RequireInitialization = 1,
		ReadCost = 6,
		AddCost = 150, // TODO(ochafik): reduce this
		MulCost = 100  // TODO(ochafik): reduce this
	};
};

namespace internal {
template <>
struct significant_decimals_impl<FilteredNumber<CGAL::Gmpq>> {
	static inline int run() { return 0; }
};
} // namespace internal
} // namespace Eigen

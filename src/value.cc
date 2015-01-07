/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "value.h"
#include "printutils.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/format.hpp>
#include "boost-utils.h"
#include "boosty.h"
/*Unicode support for string lengths and array accesses*/
#include <glib.h>

#include <boost/math/special_functions/fpclassify.hpp>

Value Value::undefined;
ValuePtr ValuePtr::undefined;

std::ostream &operator<<(std::ostream &stream, const Filename &filename)
{
  fs::path fnpath = fs::path( (std::string)filename );
  fs::path fpath = boostfs_uncomplete(fnpath, fs::current_path());
  stream << QuotedString(boosty::stringy( fpath ));
  return stream;
}

// FIXME: This could probably be done more elegantly using boost::regex
std::ostream &operator<<(std::ostream &stream, const QuotedString &s)
{
  stream << '"';
  BOOST_FOREACH(char c, s) {
    switch (c) {
    case '\t':
      stream << "\\t";
      break;
    case '\n':
      stream << "\\n";
      break;
    case '\r':
      stream << "\\r";
      break;
    case '"':
    case '\\':
      stream << '\\';
      stream << c;
      break;
    default:
      stream << c;
    }
  }
  stream << '"';
  return stream;
}

Value::Value() : value(boost::blank())
{
  //  std::cout << "creating undef\n";
}

Value::Value(bool v) : value(v)
{
  //  std::cout << "creating bool\n";
}

Value::Value(int v) : value(double(v))
{
  //  std::cout << "creating int\n";
}

Value::Value(double v) : value(v)
{
  //  std::cout << "creating double " << v << "\n";
}

Value::Value(const std::string &v) : value(v)
{
  //  std::cout << "creating string\n";
}

Value::Value(const char *v) : value(std::string(v))
{
  //  std::cout << "creating string from char *\n";
}

Value::Value(char v) : value(std::string(1, v))
{
  //  std::cout << "creating string from char\n";
}

Value::Value(const VectorType &v) : value(v)
{
  //  std::cout << "creating vector\n";
}

Value::Value(const RangeType &v) : value(v)
{
  //  std::cout << "creating range\n";
}

Value::ValueType Value::type() const
{
  return static_cast<ValueType>(this->value.which());
}

bool Value::isDefined() const
{
  return this->type() != UNDEFINED;
}

bool Value::isDefinedAs(const ValueType type) const
{
  return this->type() == type;
}

bool Value::isUndefined() const
{
  return !isDefined();
}

bool Value::toBool() const
{
  switch (this->type()) {
  case BOOL:
    return boost::get<bool>(this->value);
    break;
  case NUMBER:
    return boost::get<double>(this->value)!= 0;
    break;
  case STRING:
    return boost::get<std::string>(this->value).size() > 0;
    break;
  case VECTOR:
    return boost::get<VectorType >(this->value).size() > 0;
    break;
  case RANGE:
    return true;
    break;
  default:
    return false;
    break;
  }
}

double Value::toDouble() const
{
  double d = 0;
  getDouble(d);
  return d;
}

bool Value::getDouble(double &v) const
{
  const double *d = boost::get<double>(&this->value);
  if (d) {
    v = *d;
    return true;
  }
  return false;
}

class tostring_visitor : public boost::static_visitor<std::string>
{
public:
  template <typename T> std::string operator()(const T &op1) const {
    //    std::cout << "[generic tostring_visitor]\n";
    return boost::lexical_cast<std::string>(op1);	
  }

  std::string operator()(const double &op1) const {
    if (op1 != op1) { // Fix for avoiding nan vs. -nan across platforms
      return "nan";
    }
    if (op1 == 0) {
      return "0"; // Don't return -0 (exactly -0 and 0 equal 0)
    }
#ifdef OPENSCAD_TESTING
    // Quick and dirty hack to work around floating point rounding differences
    // across platforms for testing purposes.
    std::stringstream tmp;
    tmp.precision(12);
    tmp.setf(std::ios_base::fixed);
    tmp << op1;
    std::string tmpstr = tmp.str();
    size_t endpos = tmpstr.find_last_not_of('0');
    if (endpos >= 0 && tmpstr[endpos] == '.') endpos--;
    tmpstr = tmpstr.substr(0, endpos+1);
    size_t dotpos = tmpstr.find('.');
    if (dotpos != std::string::npos) {
      if (tmpstr.size() - dotpos > 12) tmpstr.erase(dotpos + 12);
      while (tmpstr[tmpstr.size()-1] == '0') tmpstr.erase(tmpstr.size()-1);
    }
    if ( tmpstr.compare("-0") == 0 ) tmpstr = "0";
    tmpstr = two_digit_exp_format( tmpstr );
    return tmpstr;
#else
    // attempt to emulate Qt's QString.sprintf("%g"); from old OpenSCAD.
    // see https://github.com/openscad/openscad/issues/158
    std::stringstream tmp;
    tmp.unsetf(std::ios::floatfield);
    tmp << op1;
    return tmp.str();
#endif
  }

  std::string operator()(const boost::blank &) const {
    return "undef";
  }

  std::string operator()(const bool &v) const {
    return v ? "true" : "false";
  }

  std::string operator()(const Value::VectorType &v) const {
    std::stringstream stream;
    stream << '[';
    for (size_t i = 0; i < v.size(); i++) {
      if (i > 0) stream << ", ";
      stream << v[i];
    }
    stream << ']';
    return stream.str();
  }

  std::string operator()(const Value::RangeType &v) const {
    return (boost::format("[%1% : %2% : %3%]") % v.begin_val % v.step_val % v.end_val).str();
  }
};

std::string Value::toString() const
{
  return boost::apply_visitor(tostring_visitor(), this->value);
}

class chr_visitor : public boost::static_visitor<std::string> {
public:
	template <typename S> std::string operator()(const S &) const
	{
		return "";
	}

	std::string operator()(const double &v) const
	{
		char buf[8];
		memset(buf, 0, 8);
		if (v > 0) {
			const gunichar c = v;
			if (g_unichar_validate(c) && (c != 0)) {
			    g_unichar_to_utf8(c, buf);
			}
		}
		return std::string(buf);
	}

	std::string operator()(const Value::VectorType &v) const
	{
		std::stringstream stream;
		for (size_t i = 0; i < v.size(); i++) {
			stream << v[i].chrString();
		}
		return stream.str();
	}

	std::string operator()(const Value::RangeType &v) const
	{
		const boost::uint32_t steps = v.nbsteps();
		if (steps >= 10000) {
			PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu).", steps);
			return "";
		}

		std::stringstream stream;
		Value::RangeType range = v;
		for (Value::RangeType::iterator it = range.begin();it != range.end();it++) {
			const Value value(*it);
			stream << value.chrString();
		}
		return stream.str();
	}
};

std::string Value::chrString() const
{
  return boost::apply_visitor(chr_visitor(), this->value);
}

const Value::VectorType &Value::toVector() const
{
  static VectorType empty;
  
  const VectorType *v = boost::get<VectorType>(&this->value);
  if (v) return *v;
  else return empty;
}

bool Value::getVec2(double &x, double &y) const
{
  if (this->type() != VECTOR) return false;

  const VectorType &v = toVector();
  
  if (v.size() != 2) return false;
  return (v[0].getDouble(x) && v[1].getDouble(y));
}

bool Value::getVec3(double &x, double &y, double &z, double defaultval) const
{
  if (this->type() != VECTOR) return false;

  const VectorType &v = toVector();

  if (v.size() == 2) {
    getVec2(x, y);
    z = defaultval;
    return true;
  }
  else {
    if (v.size() != 3) return false;
  }

  return (v[0].getDouble(x) && v[1].getDouble(y) && v[2].getDouble(z));
}

Value::RangeType Value::toRange() const
{
  const RangeType *val = boost::get<RangeType>(&this->value);
  if (val) {
    return *val;
  }
  else return RangeType(0,0,0);
}

Value &Value::operator=(const Value &v)
{
  if (this != &v) {
    this->value = v.value;
  }
  return *this;
}

class equals_visitor : public boost::static_visitor<bool>
{
public:
  template <typename T, typename U> bool operator()(const T &, const U &) const {
    return false;
  }

  template <typename T> bool operator()(const T &op1, const T &op2) const {
    return op1 == op2;
  }
};

bool Value::operator==(const Value &v) const
{
  return boost::apply_visitor(equals_visitor(), this->value, v.value);
}

bool Value::operator!=(const Value &v) const
{
  return !(*this == v);
}

#define DEFINE_VISITOR(name,op)\
class name : public boost::static_visitor<bool> \
{ \
public:\
  template <typename T, typename U> bool operator()(const T &, const U &) const {\
    return false;\
  }\
\
  bool operator()(const bool &op1, const bool &op2) const {\
    return op1 op op2;\
  }\
\
  bool operator()(const bool &op1, const double &op2) const {\
    return op1 op op2;\
  }\
\
  bool operator()(const double &op1, const bool &op2) const {\
    return op1 op op2;\
  }\
\
  bool operator()(const double &op1, const double &op2) const {\
    return op1 op op2;\
  }\
\
  bool operator()(const std::string &op1, const std::string &op2) const {\
    return op1 op op2;\
  }\
}

DEFINE_VISITOR(less_visitor, <);
DEFINE_VISITOR(greater_visitor, >);
DEFINE_VISITOR(lessequal_visitor, <=);
DEFINE_VISITOR(greaterequal_visitor, >=);

bool Value::operator<(const Value &v) const
{
  return boost::apply_visitor(less_visitor(), this->value, v.value);
}

bool Value::operator>=(const Value &v) const
{
  return boost::apply_visitor(greaterequal_visitor(), this->value, v.value);
}

bool Value::operator>(const Value &v) const
{
  return boost::apply_visitor(greater_visitor(), this->value, v.value);
}

bool Value::operator<=(const Value &v) const
{
  return boost::apply_visitor(lessequal_visitor(), this->value, v.value);
}

class plus_visitor : public boost::static_visitor<Value>
{
public:
  template <typename T, typename U> Value operator()(const T &, const U &) const {
    return Value::undefined;
  }

  Value operator()(const double &op1, const double &op2) const {
    return Value(op1 + op2);
  }

  Value operator()(const Value::VectorType &op1, const Value::VectorType &op2) const {
    Value::VectorType sum;
    for (size_t i = 0; i < op1.size() && i < op2.size(); i++) {
      sum.push_back(op1[i] + op2[i]);
    }
    return Value(sum);
  }
};

Value Value::operator+(const Value &v) const
{
  return boost::apply_visitor(plus_visitor(), this->value, v.value);
}

class minus_visitor : public boost::static_visitor<Value>
{
public:
  template <typename T, typename U> Value operator()(const T &, const U &) const {
    return Value::undefined;
  }

  Value operator()(const double &op1, const double &op2) const {
    return Value(op1 - op2);
  }

  Value operator()(const Value::VectorType &op1, const Value::VectorType &op2) const {
    Value::VectorType sum;
    for (size_t i = 0; i < op1.size() && i < op2.size(); i++) {
      sum.push_back(op1[i] - op2[i]);
    }
    return Value(sum);
  }
};

Value Value::operator-(const Value &v) const
{
  return boost::apply_visitor(minus_visitor(), this->value, v.value);
}

Value Value::multvecnum(const Value &vecval, const Value &numval)
{
  // Vector * Number
  VectorType dstv;
  BOOST_FOREACH(const Value &val, vecval.toVector()) {
    dstv.push_back(val * numval);
  }
  return Value(dstv);
}

Value Value::multmatvec(const Value &matrixval, const Value &vectorval)
{
  const VectorType &matrixvec = matrixval.toVector();
  const VectorType &vectorvec = vectorval.toVector();

  // Matrix * Vector
  VectorType dstv;
  for (size_t i=0;i<matrixvec.size();i++) {
    if (matrixvec[i].type() != VECTOR || 
        matrixvec[i].toVector().size() != vectorvec.size()) {
      return Value();
    }
    double r_e = 0.0;
    for (size_t j=0;j<matrixvec[i].toVector().size();j++) {
      if (matrixvec[i].toVector()[j].type() != NUMBER || vectorvec[j].type() != NUMBER) {
        return Value();
      }
      r_e += matrixvec[i].toVector()[j].toDouble() * vectorvec[j].toDouble();
    }
    dstv.push_back(Value(r_e));
  }
  return Value(dstv);
}

Value Value::multvecmat(const Value &vectorval, const Value &matrixval)
{
  const VectorType &vectorvec = vectorval.toVector();
  const VectorType &matrixvec = matrixval.toVector();
  assert(vectorvec.size() == matrixvec.size());
  // Vector * Matrix
  VectorType dstv;
  for (size_t i=0;i<matrixvec[0].toVector().size();i++) {
    double r_e = 0.0;
    for (size_t j=0;j<vectorvec.size();j++) {
      if (matrixvec[j].type() != VECTOR ||
          matrixvec[j].toVector()[i].type() != NUMBER || 
          vectorvec[j].type() != NUMBER) {
        return Value::undefined;
      }
      r_e += vectorvec[j].toDouble() * matrixvec[j].toVector()[i].toDouble();
    }
    dstv.push_back(Value(r_e));
  }
  return Value(dstv);
}

Value Value::operator*(const Value &v) const
{
  if (this->type() == NUMBER && v.type() == NUMBER) {
    return Value(this->toDouble() * v.toDouble());
  }
  else if (this->type() == VECTOR && v.type() == NUMBER) {
    return multvecnum(*this, v);
  }
  else if (this->type() == NUMBER && v.type() == VECTOR) {
    return multvecnum(v, *this);
  }
  else if (this->type() == VECTOR && v.type() == VECTOR) {
    const VectorType &vec1 = this->toVector();
    const VectorType &vec2 = v.toVector();
    if (vec1[0].type() == NUMBER && vec2[0].type() == NUMBER &&
        vec1.size() == vec2.size()) { 
        // Vector dot product.
        double r = 0.0;
        for (size_t i=0;i<vec1.size();i++) {
          if (vec1[i].type() != NUMBER || vec2[i].type() != NUMBER) {
            return Value::undefined;
          }
          r += (vec1[i].toDouble() * vec2[i].toDouble());
        }
        return Value(r);
    } else if (vec1[0].type() == VECTOR && vec2[0].type() == NUMBER &&
               vec1[0].toVector().size() == vec2.size()) {
      return multmatvec(vec1, vec2);
    } else if (vec1[0].type() == NUMBER && vec2[0].type() == VECTOR &&
               vec1.size() == vec2.size()) {
      return multvecmat(vec1, vec2);
    } else if (vec1[0].type() == VECTOR && vec2[0].type() == VECTOR &&
               vec1[0].toVector().size() == vec2.size()) {
      // Matrix * Matrix
      VectorType dstv;
      BOOST_FOREACH(const Value &srcrow, vec1) {
        dstv.push_back(multvecmat(srcrow, vec2));
      }
      return Value(dstv);
    }
  }
  return Value::undefined;
}

Value Value::operator/(const Value &v) const
{
  if (this->type() == NUMBER && v.type() == NUMBER) {
    return Value(this->toDouble() / v.toDouble());
  }
  else if (this->type() == VECTOR && v.type() == NUMBER) {
    const VectorType &vec = this->toVector();
    VectorType dstv;
    BOOST_FOREACH(const Value &vecval, vec) {
      dstv.push_back(vecval / v);
    }
    return Value(dstv);
  }
  else if (this->type() == NUMBER && v.type() == VECTOR) {
    const VectorType &vec = v.toVector();
    VectorType dstv;
    BOOST_FOREACH(const Value &vecval, vec) {
      dstv.push_back(*this / vecval);
    }
    return Value(dstv);
  }
  return Value::undefined;
}

Value Value::operator%(const Value &v) const
{
  if (this->type() == NUMBER && v.type() == NUMBER) {
    return Value(fmod(boost::get<double>(this->value), boost::get<double>(v.value)));
  }
  return Value::undefined;
}

Value Value::operator-() const
{
  if (this->type() == NUMBER) {
    return Value(-this->toDouble());
  }
  else if (this->type() == VECTOR) {
    const VectorType &vec = this->toVector();
    VectorType dstv;
    BOOST_FOREACH(const Value &vecval, vec) {
      dstv.push_back(-vecval);
    }
    return Value(dstv);
  }
  return Value::undefined;
}

/*!
  Append a value to this vector.
  This must be of valtype VECTOR.
*/
/*
  void Value::append(Value *val)
  {
  assert(this->type() == VECTOR);
  this->vec.push_back(val);
  }
*/

/*
 * bracket operation [] detecting multi-byte unicode.
 * If the string is multi-byte unicode then the index will offset to the character (2 or 4 byte) and not to the byte.
 * A 'normal' string with byte chars are a subset of unicode and still work.
 */
class bracket_visitor : public boost::static_visitor<Value>
{
public:
  Value operator()(const std::string &str, const double &idx) const {
    int i = int(idx);
    Value v;
    //Check that the index is positive and less than the size in bytes
    if ((i >= 0) && (i < (int)str.size())) {
	  //Ensure character (not byte) index is inside the character/glyph array
	  if( (unsigned) i < g_utf8_strlen( str.c_str(), str.size() ) )	{
		  gchar utf8_of_cp[6] = ""; //A buffer for a single unicode character to be copied into
		  gchar* ptr = g_utf8_offset_to_pointer(str.c_str(), i);
		  if(ptr) {
		    g_utf8_strncpy(utf8_of_cp, ptr, 1);
		  }
		  v = std::string(utf8_of_cp);
	  }
      //      std::cout << "bracket_visitor: " <<  v << "\n";
    }
    return v;
  }

  Value operator()(const Value::VectorType &vec, const double &idx) const {
    int i = int(idx);
    if ((i >= 0) && (i < (int)vec.size())) return vec[int(idx)];
    return Value::undefined;
  }

  Value operator()(const Value::RangeType &range, const double &idx) const {
    switch(int(idx)) {
    case 0: return Value(range.begin_val);
    case 1: return Value(range.step_val);
    case 2: return Value(range.end_val);
    }
    return Value::undefined;
  }

  template <typename T, typename U> Value operator()(const T &, const U &) const {
    //    std::cout << "generic bracket_visitor\n";
    return Value::undefined;
  }
};

Value Value::operator[](const Value &v) const
{
  return boost::apply_visitor(bracket_visitor(), this->value, v.value);
}

void Value::RangeType::normalize() {
  if ((step_val>0) && (end_val < begin_val)) {
    std::swap(begin_val,end_val);
    printDeprecation("Using ranges of the form [begin:end] with begin value greater than the end value is deprecated.");
  }
}

boost::uint32_t Value::RangeType::nbsteps() const {
  if (boost::math::isnan(step_val) || boost::math::isinf(begin_val) || (boost::math::isinf(end_val))) {
    return std::numeric_limits<boost::uint32_t>::max();
  }

  if ((begin_val == end_val) || boost::math::isinf(step_val)) {
    return 0;
  }
  
  if (step_val == 0) { 
    return std::numeric_limits<boost::uint32_t>::max();
  }

  double steps;
  if (step_val < 0) {
    if (begin_val < end_val) {
      return 0;
    }
    steps = (begin_val - end_val) / (-step_val);
  } else {
    if (begin_val > end_val) {
      return 0;
    }
    steps = (end_val - begin_val) / step_val;
  }
  
  return steps;
}

Value::RangeType::iterator::iterator(Value::RangeType &range, type_t type) : range(range), val(range.begin_val)
{
    this->type = type;
    update_type();
}

void Value::RangeType::iterator::update_type()
{
    if (range.step_val == 0) {
        type = RANGE_TYPE_END;
    } else if (range.step_val < 0) {
        if (val < range.end_val) {
            type = RANGE_TYPE_END;
        }
    } else {
        if (val > range.end_val) {
            type = RANGE_TYPE_END;
        }
    }
}

Value::RangeType::iterator::reference Value::RangeType::iterator::operator*()
{
    return val;
}

Value::RangeType::iterator::pointer Value::RangeType::iterator::operator->()
{
    return &(operator*());
}

Value::RangeType::iterator::self_type Value::RangeType::iterator::operator++()
{
    if (type < 0) {
        type = RANGE_TYPE_RUNNING;
    }
    val += range.step_val;
    update_type();
    return *this;
}

Value::RangeType::iterator::self_type Value::RangeType::iterator::operator++(int)
{
    self_type tmp(*this);
    operator++();
    return tmp;
}

bool Value::RangeType::iterator::operator==(const self_type &other) const
{
    if (type == RANGE_TYPE_RUNNING) {
        return (type == other.type) && (val == other.val) && (range == other.range);
    } else {
        return (type == other.type) && (range == other.range);
    }
}

bool Value::RangeType::iterator::operator!=(const self_type &other) const
{
    return !(*this == other);
}

ValuePtr::ValuePtr()
{
	this->reset(new Value());
}

ValuePtr::ValuePtr(const Value &v)
{
	this->reset(new Value(v));
}

ValuePtr::ValuePtr(bool v)
{
	this->reset(new Value(v));
}

ValuePtr::ValuePtr(int v)
{
	this->reset(new Value(v));
}

ValuePtr::ValuePtr(double v)
{
	this->reset(new Value(v));
}

ValuePtr::ValuePtr(const std::string &v)
{
	this->reset(new Value(v));
}

ValuePtr::ValuePtr(const char *v)
{
	this->reset(new Value(v));
}

ValuePtr::ValuePtr(const char v)
{
	this->reset(new Value(v));
}

ValuePtr::ValuePtr(const Value::VectorType &v)
{
	this->reset(new Value(v));
}

ValuePtr::ValuePtr(const Value::RangeType &v)
{
	this->reset(new Value(v));
}

bool ValuePtr::operator==(const ValuePtr &v) const
{
	return ValuePtr(**this == *v);
}

bool ValuePtr::operator!=(const ValuePtr &v) const
{
	return ValuePtr(**this != *v);
}

bool ValuePtr::operator<(const ValuePtr &v) const
{
	return ValuePtr(**this < *v);
}

bool ValuePtr::operator<=(const ValuePtr &v) const
{
	return ValuePtr(**this <= *v);
}

bool ValuePtr::operator>=(const ValuePtr &v) const
{
	return ValuePtr(**this >= *v);
}

bool ValuePtr::operator>(const ValuePtr &v) const
{
	return ValuePtr(**this > *v);
}

ValuePtr ValuePtr::operator-() const
{
	return ValuePtr(-**this);
}

ValuePtr ValuePtr::operator!() const
{
	return ValuePtr(!**this);
}

ValuePtr ValuePtr::operator[](const ValuePtr &v) const
{
	return ValuePtr((**this)[*v]);
}

ValuePtr ValuePtr::operator+(const ValuePtr &v) const
{
	return ValuePtr(**this + *v);
}

ValuePtr ValuePtr::operator-(const ValuePtr &v) const
{
	return ValuePtr(**this - *v);
}

ValuePtr ValuePtr::operator*(const ValuePtr &v) const
{
	return ValuePtr(**this * *v);
}

ValuePtr ValuePtr::operator/(const ValuePtr &v) const
{
	return ValuePtr(**this / *v);
}

ValuePtr ValuePtr::operator%(const ValuePtr &v) const
{
	return ValuePtr(**this % *v);
}


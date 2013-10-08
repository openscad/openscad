#ifndef VALUE_H_
#define VALUE_H_

#include <vector>
#include <string>
#include <algorithm>
#include <limits>

// Workaround for https://bugreports.qt-project.org/browse/QTBUG-22829
#ifndef Q_MOC_RUN
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#endif

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
  struct RangeType {
    RangeType(double begin, double step, double end)
      : begin(begin), step(step), end(end) {}

    bool operator==(const RangeType &other) const {
      return this->begin == other.begin &&
        this->step == other.step &&
        this->end == other.end;
    }

    /// inverse begin/end if begin is upper than end
    void normalize() {
		if ((step>0) && (end < begin)) {
			std::swap(begin,end);
		}
	}
	/// return number of steps, max int value if step is null
	int nbsteps() const {
		if (step<=0) {
			return std::numeric_limits<int>::max();
		}
		return (int)((begin-end)/step);
	}

    double begin;
    double step;
    double end;
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
  bool isUndefined() const;

  double toDouble() const;
  bool getDouble(double &v) const;
  bool toBool() const;
  std::string toString() const;
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
  Value operator[](const Value &v);
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

#endif

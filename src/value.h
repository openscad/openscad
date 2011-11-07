#ifndef VALUE_H_
#define VALUE_H_

#include <vector>
#include <string>

class Filename : public std::string
{
public:
	Filename() : std::string() {}
	Filename(const std::string &f) : std::string(f) {}
};

class Value
{
public:
	enum type_e {
		UNDEFINED,
		BOOL,
		NUMBER,
		RANGE,
		VECTOR,
		STRING
	};

	enum type_e type;

	bool b;
	double num;
	std::vector<Value*> vec;
	double range_begin;
	double range_step;
	double range_end;
	std::string text;

	Value();
	~Value();

	Value(bool v);
	Value(double v);
	Value(const std::string &t);

	Value(const Value &v);
	Value& operator = (const Value &v);

	Value operator ! () const;
	Value operator && (const Value &v) const;
	Value operator || (const Value &v) const;

	Value operator + (const Value &v) const;
	Value operator - (const Value &v) const;
	Value operator * (const Value &v) const;
	Value operator / (const Value &v) const;
	Value operator % (const Value &v) const;

	Value operator < (const Value &v) const;
	Value operator <= (const Value &v) const;
	Value operator == (const Value &v) const;
	Value operator != (const Value &v) const;
	Value operator >= (const Value &v) const;
	Value operator > (const Value &v) const;

	Value inv() const;

	bool getnum(double &v) const;
	bool getv2(double &x, double &y) const;
	bool getv3(double &x, double &y, double &z) const;

	std::string toString() const;

	bool toBool() const;

	void append(Value *val);

private:
	void reset_undef();
};

std::ostream &operator<<(std::ostream &stream, const Value &value);
std::ostream &operator<<(std::ostream &stream, const Filename &filename);

#endif

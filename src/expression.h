#pragma once

#include <string>
#include <vector>
#include "value.h"
#include "typedefs.h"

class Expression
{
public:
	std::vector<Expression*> children;
	Expression *first;
	Expression *second;
	Expression *third;

	ValuePtr const_value;
	std::string var_name;

	std::string call_funcname;
	AssignmentList call_arguments;

	Expression();
	Expression(const ValuePtr &val);
	Expression(const std::string &val);
	Expression(const std::string &val, Expression *expr);
	Expression(Expression *expr);
	Expression(Expression *left, Expression *right);
	Expression(Expression *expr1, Expression *expr2, Expression *expr3);
	virtual ~Expression();

	virtual bool isListComprehension() const;
	virtual ValuePtr evaluate(const class Context *context) const = 0;
	virtual void print(std::ostream &stream) const = 0;
};

std::ostream &operator<<(std::ostream &stream, const Expression &expr);

class ExpressionNot : public Expression
{
public:
	ExpressionNot(Expression *expr);
	virtual ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionLogicalAnd : public Expression
{
public:
	ExpressionLogicalAnd(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionLogicalOr : public Expression
{
public:
	ExpressionLogicalOr(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionMultiply : public Expression
{
public:
	ExpressionMultiply(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionDivision : public Expression
{
public:
	ExpressionDivision(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionModulo : public Expression
{
public:
	ExpressionModulo(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionPlus : public Expression
{
public:
	ExpressionPlus(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionMinus : public Expression
{
public:
	ExpressionMinus(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionLess : public Expression
{
public:
	ExpressionLess(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionLessOrEqual : public Expression
{
public:
	ExpressionLessOrEqual(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionEqual : public Expression
{
public:
	ExpressionEqual(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionNotEqual : public Expression
{
public:
	ExpressionNotEqual(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionGreaterOrEqual : public Expression
{
public:
	ExpressionGreaterOrEqual(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionGreater : public Expression
{
public:
	ExpressionGreater(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionTernary : public Expression
{
public:
	ExpressionTernary(Expression *expr1, Expression *expr2, Expression *expr3);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionArrayLookup : public Expression
{
public:
	ExpressionArrayLookup(Expression *left, Expression *right);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionInvert : public Expression
{
public:
	ExpressionInvert(Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionConst : public Expression
{
public:
	ExpressionConst(const ValuePtr &val);
	ValuePtr evaluate(const class Context *) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionRange : public Expression
{
public:
	ExpressionRange(Expression *expr1, Expression *expr2);
	ExpressionRange(Expression *expr1, Expression *expr2, Expression *expr3);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionVector : public Expression
{
public:
	ExpressionVector(Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionLookup : public Expression
{
public:
	ExpressionLookup(const std::string &val);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionMember : public Expression
{
public:
	ExpressionMember(const std::string &val, Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionFunction : public Expression
{
public:
	ExpressionFunction();
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionLet : public Expression
{
public:
	ExpressionLet(Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionLcExpression : public Expression
{
public:
	ExpressionLcExpression(Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

class ExpressionLc : public Expression
{
	virtual bool isListComprehension() const;
public:
	ExpressionLc(Expression *expr);
	ExpressionLc(Expression *expr1, Expression *expr2);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
};

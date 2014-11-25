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

	Expression();
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
private:
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
private:
	ValuePtr const_value;
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
	ExpressionLookup(const std::string &var_name);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	std::string var_name;
};

class ExpressionMember : public Expression
{
public:
	ExpressionMember(Expression *expr, const std::string &member);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	std::string member;
};

class ExpressionFunctionCall : public Expression
{
public:
	ExpressionFunctionCall(const std::string &funcname, const AssignmentList &arglist);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
public:
	std::string funcname;
	AssignmentList call_arguments;
};

class ExpressionLet : public Expression
{
public:
	ExpressionLet(const AssignmentList &arglist, Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	AssignmentList call_arguments;
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
	ExpressionLc(const std::string &name, 
							 const AssignmentList &arglist, Expression *expr);
	ExpressionLc(const std::string &name, 
							 Expression *expr1, Expression *expr2);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	std::string name;
	AssignmentList call_arguments;
};

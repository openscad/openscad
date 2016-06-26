#pragma once

#include "AST.h"

#include <string>
#include <vector>
#include "value.h"
#include "memory.h"
#include "Assignment.h"

class Expression : public ASTNode
{
public:
	Expression();
	virtual ~Expression();

	virtual bool isListComprehension() const;
	virtual ValuePtr evaluate(const class Context *context) const = 0;
	virtual void print(std::ostream &stream) const = 0;
};

std::ostream &operator<<(std::ostream &stream, const Expression &expr);

class UnaryOp : public Expression
{
public:
	enum class Op {
		Not,
		Negate
	};

	UnaryOp(Op op, Expression *expr);
	virtual ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;

private:
	const char *opString() const;

	Op op;
	shared_ptr<Expression> expr;
};

class BinaryOp : public Expression
{
public:
	enum class Op {
		LogicalAnd,
		LogicalOr,
		Multiply,
		Divide,
		Modulo,
		Plus,
		Minus,
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Equal,
		NotEqual
	};

	BinaryOp(Expression *left, Op op, Expression *right);
	virtual ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;

private:
	const char *opString() const;

	Op op;
	shared_ptr<Expression> left;
	shared_ptr<Expression> right;
};

class TernaryOp : public Expression
{
public:
	TernaryOp(Expression *cond, Expression *ifexpr, Expression *elseexpr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;

	shared_ptr<Expression> cond;
	shared_ptr<Expression> ifexpr;
	shared_ptr<Expression> elseexpr;
};

class ArrayLookup : public Expression
{
public:
	ArrayLookup(Expression *array, Expression *index);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	shared_ptr<Expression> array;
	shared_ptr<Expression> index;
};

class Literal : public Expression
{
public:
	Literal(const ValuePtr &val);
	ValuePtr evaluate(const class Context *) const;
	virtual void print(std::ostream &stream) const;
private:
	ValuePtr value;
};

class Range : public Expression
{
public:
	Range(Expression *begin, Expression *end);
	Range(Expression *begin, Expression *step, Expression *end);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	shared_ptr<Expression> begin;
	shared_ptr<Expression> step;
	shared_ptr<Expression> end;
};

class Vector : public Expression
{
public:
	Vector();
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
	void push_back(Expression *expr);
private:
	std::vector<shared_ptr<Expression>> children;
};

class Lookup : public Expression
{
public:
	Lookup(const std::string &name);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	std::string name;
};

class MemberLookup : public Expression
{
public:
	MemberLookup(Expression *expr, const std::string &member);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	shared_ptr<Expression> expr;
	std::string member;
};

class FunctionCall : public Expression
{
public:
	FunctionCall(const std::string &funcname, const AssignmentList &arglist);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
public:
	std::string name;
	AssignmentList arguments;
};

class Let : public Expression
{
public:
	Let(const AssignmentList &args, Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

class ListComprehension : public Expression
{
	virtual bool isListComprehension() const;
public:
	ListComprehension();
};

class LcIf : public ListComprehension
{
public:
	LcIf(Expression *cond, Expression *ifexpr, Expression *elseexpr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	shared_ptr<Expression> cond;
	shared_ptr<Expression> ifexpr;
	shared_ptr<Expression> elseexpr;
};

class LcFor : public ListComprehension
{
public:
	LcFor(const AssignmentList &args, Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

class LcForC : public ListComprehension
{
public:
	LcForC(const AssignmentList &args, const AssignmentList &incrargs, Expression *cond, Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	AssignmentList arguments;
	AssignmentList incr_arguments;
	shared_ptr<Expression> cond;
	shared_ptr<Expression> expr;
};

class LcEach : public ListComprehension
{
public:
	LcEach(Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	shared_ptr<Expression> expr;
};

class LcLet : public ListComprehension
{
public:
	LcLet(const AssignmentList &args, Expression *expr);
	ValuePtr evaluate(const class Context *context) const;
	virtual void print(std::ostream &stream) const;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

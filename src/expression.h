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
	Expression(const Location &loc) : ASTNode(loc) {}
	~Expression() {}
	virtual bool isLiteral() const;
	virtual ValuePtr evaluate(const class Context *context) const = 0;
};

class UnaryOp : public Expression
{
public:
	enum class Op {
		Not,
		Negate
	};
	bool isLiteral() const override;
	UnaryOp(Op op, Expression *expr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;

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

	BinaryOp(Expression *left, Op op, Expression *right, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;

private:
	const char *opString() const;

	Op op;
	shared_ptr<Expression> left;
	shared_ptr<Expression> right;
};

class TernaryOp : public Expression
{
public:
	TernaryOp(Expression *cond, Expression *ifexpr, Expression *elseexpr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;

	shared_ptr<Expression> cond;
	shared_ptr<Expression> ifexpr;
	shared_ptr<Expression> elseexpr;
};

class ArrayLookup : public Expression
{
public:
	ArrayLookup(Expression *array, Expression *index, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	shared_ptr<Expression> array;
	shared_ptr<Expression> index;
};

class Literal : public Expression
{
public:
	Literal(const ValuePtr &val, const Location &loc = Location::NONE);
	ValuePtr evaluate(const class Context *) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	bool isLiteral() const override { return true;}
private:
	ValuePtr value;
};

class Range : public Expression
{
public:
	Range(Expression *begin, Expression *end, const Location &loc);
	Range(Expression *begin, Expression *step, Expression *end, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	bool isLiteral() const override;
private:
	shared_ptr<Expression> begin;
	shared_ptr<Expression> step;
	shared_ptr<Expression> end;
};

class Vector : public Expression
{
public:
	Vector(const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	void push_back(Expression *expr);
	bool isLiteral() const override;
private:
	std::vector<shared_ptr<Expression>> children;
};

class Lookup : public Expression
{
public:
	Lookup(const std::string &name, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	std::string name;
};

class MemberLookup : public Expression
{
public:
	MemberLookup(Expression *expr, const std::string &member, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	shared_ptr<Expression> expr;
	std::string member;
};

class FunctionCall : public Expression
{
public:
	FunctionCall(const std::string &funcname, const AssignmentList &arglist, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	static Expression * create(const std::string &funcname, const AssignmentList &arglist, Expression *expr, const Location &loc);
public:
	std::string name;
	AssignmentList arguments;
};

class Assert : public Expression
{
public:
	Assert(const AssignmentList &args, Expression *expr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

class Echo : public Expression
{
public:
	Echo(const AssignmentList &args, Expression *expr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

class Let : public Expression
{
public:
	Let(const AssignmentList &args, Expression *expr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

class ListComprehension : public Expression
{
public:
	ListComprehension(const Location &loc);
	~ListComprehension() = default;
};

class LcIf : public ListComprehension
{
public:
	LcIf(Expression *cond, Expression *ifexpr, Expression *elseexpr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	shared_ptr<Expression> cond;
	shared_ptr<Expression> ifexpr;
	shared_ptr<Expression> elseexpr;
};

class LcFor : public ListComprehension
{
public:
	LcFor(const AssignmentList &args, Expression *expr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

class LcForC : public ListComprehension
{
public:
	LcForC(const AssignmentList &args, const AssignmentList &incrargs, Expression *cond, Expression *expr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	AssignmentList incr_arguments;
	shared_ptr<Expression> cond;
	shared_ptr<Expression> expr;
};

class LcEach : public ListComprehension
{
public:
	LcEach(Expression *expr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	shared_ptr<Expression> expr;
};

class LcLet : public ListComprehension
{
public:
	LcLet(const AssignmentList &args, Expression *expr, const Location &loc);
	ValuePtr evaluate(const class Context *context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

void evaluate_assert(const Context &context, const class EvalContext *evalctx, const Location &loc);

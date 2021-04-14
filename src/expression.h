#pragma once

#include <functional>
#include <string>
#include <vector>
#include "Assignment.h"
#include "boost-utils.h"
#include "function.h"
#include "memory.h"
#include "value.h"

template<class T> class ContextHandle;

class Expression : public ASTNode
{
public:
	Expression(const Location &loc) : ASTNode(loc) {}
	~Expression() {}
	virtual bool isLiteral() const;
	virtual Value evaluate(const std::shared_ptr<Context>& context) const = 0;
	Value checkUndef(Value&& val, const std::shared_ptr<Context>& context) const;
	Value evaluateLiteral() const;
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
	Value evaluate(const std::shared_ptr<Context>& context) const override;
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
		Exponent,
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
	Value evaluate(const std::shared_ptr<Context>& context) const override;
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
	const Expression* evaluateStep(const std::shared_ptr<Context>& context) const;
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	shared_ptr<Expression> cond;
	shared_ptr<Expression> ifexpr;
	shared_ptr<Expression> elseexpr;
};

class ArrayLookup : public Expression
{
public:
	ArrayLookup(Expression *array, Expression *index, const Location &loc);
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	shared_ptr<Expression> array;
	shared_ptr<Expression> index;
};

class Literal : public Expression
{
public:
	Literal(Value val, const Location &loc = Location::NONE);
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	bool isLiteral() const override { return true;}
private:
	const Value value;
};

class Range : public Expression
{
public:
	Range(Expression *begin, Expression *end, const Location &loc);
	Range(Expression *begin, Expression *step, Expression *end, const Location &loc);
	Value evaluate(const std::shared_ptr<Context>& context) const override;
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
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	void emplace_back(Expression *expr);
	bool isLiteral() const override;
private:
	std::vector<shared_ptr<Expression>> children;
	mutable boost::tribool literal_flag; // cache if already computed
};

class Lookup : public Expression
{
public:
	Lookup(const std::string &name, const Location &loc);
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	const std::string& get_name() const { return name; }
private:
	std::string name;
};

class MemberLookup : public Expression
{
public:
	MemberLookup(Expression *expr, const std::string &member, const Location &loc);
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	shared_ptr<Expression> expr;
	std::string member;
};

class FunctionCall : public Expression
{
public:
	FunctionCall(Expression *expr, const AssignmentList &arglist, const Location &loc);
	boost::optional<CallableFunction> evaluate_function_expression(const std::shared_ptr<Context>& context) const;
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
	const std::string& get_name() const { return name; }
	static Expression * create(const std::string &funcname, const AssignmentList &arglist, Expression *expr, const Location &loc);
public:
	bool isLookup;
	std::string name;
	shared_ptr<Expression> expr;
	AssignmentList arguments;
};

class FunctionDefinition : public Expression
{
public:
	FunctionDefinition(Expression *expr, const AssignmentList &parameters, const Location &loc);
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
public:
	shared_ptr<Context> ctx;
	AssignmentList parameters;
	shared_ptr<Expression> expr;
};

class Assert : public Expression
{
public:
	Assert(const AssignmentList &args, Expression *expr, const Location &loc);
	static void performAssert(const AssignmentList& arguments, const Location& location, const std::shared_ptr<Context>& context);
	const Expression* evaluateStep(const std::shared_ptr<Context>& context) const;
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

class Echo : public Expression
{
public:
	Echo(const AssignmentList &args, Expression *expr, const Location &loc);
	const Expression* evaluateStep(const std::shared_ptr<Context>& context) const;
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

class Let : public Expression
{
public:
	Let(const AssignmentList &args, Expression *expr, const Location &loc);
	static void doSequentialAssignment(const AssignmentList& assignments, const Location& location, const std::shared_ptr<Context>& targetContext);
	static ContextHandle<Context> sequentialAssignmentContext(const AssignmentList& assignments, const Location& location, const std::shared_ptr<Context>& context);
	const Expression* evaluateStep(const std::shared_ptr<Context>& targetContext) const;
	Value evaluate(const std::shared_ptr<Context>& context) const override;
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
	Value evaluate(const std::shared_ptr<Context>& context) const override;
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
	static void forEach(const AssignmentList& assignments, const Location &loc, const std::shared_ptr<Context>& context, std::function<void(const std::shared_ptr<Context>&)> operation);
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

class LcForC : public ListComprehension
{
public:
	LcForC(const AssignmentList &args, const AssignmentList &incrargs, Expression *cond, Expression *expr, const Location &loc);
	Value evaluate(const std::shared_ptr<Context>& context) const override;
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
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	Value evalRecur(Value &&v, const std::shared_ptr<Context>& context) const;
	shared_ptr<Expression> expr;
};

class LcLet : public ListComprehension
{
public:
	LcLet(const AssignmentList &args, Expression *expr, const Location &loc);
	Value evaluate(const std::shared_ptr<Context>& context) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
private:
	AssignmentList arguments;
	shared_ptr<Expression> expr;
};

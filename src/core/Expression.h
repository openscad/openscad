#pragma once

#include <ostream>
#include <utility>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <boost/logic/tribool.hpp>
#include "core/Assignment.h"
#include "core/function.h"
#include "core/Value.h"

template <class T> class ContextHandle;

class Expression : public ASTNode
{
public:
  Expression(const Location& loc) : ASTNode(loc) {}
  [[nodiscard]] virtual bool isLiteral() const;
  [[nodiscard]] virtual Value evaluate(const std::shared_ptr<const Context>& context) const = 0;
  Value checkUndef(Value&& val, const std::shared_ptr<const Context>& context) const;
};

class UnaryOp : public Expression
{
public:
  enum class Op {
    Not,
    Negate
  };
  [[nodiscard]] bool isLiteral() const override;
  UnaryOp(Op op, Expression *expr, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;

private:
  [[nodiscard]] const char *opString() const;

  Op op;
  std::shared_ptr<Expression> expr;
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

  BinaryOp(Expression *left, Op op, Expression *right, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;

private:
  [[nodiscard]] const char *opString() const;

  Op op;
  std::shared_ptr<Expression> left;
  std::shared_ptr<Expression> right;
};

class TernaryOp : public Expression
{
public:
  TernaryOp(Expression *cond, Expression *ifexpr, Expression *elseexpr, const Location& loc);
  [[nodiscard]] const Expression *evaluateStep(const std::shared_ptr<const Context>& context) const;
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  std::shared_ptr<Expression> cond;
  std::shared_ptr<Expression> ifexpr;
  std::shared_ptr<Expression> elseexpr;
};

class ArrayLookup : public Expression
{
public:
  ArrayLookup(Expression *array, Expression *index, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  std::shared_ptr<Expression> array;
  std::shared_ptr<Expression> index;
};

class Literal : public Expression
{
public:
  Literal(const Location& loc = Location::NONE) : Expression(loc), value(Value::undefined.clone()) { }
  Literal(Value val, const Location& loc = Location::NONE) : Expression(loc), value(std::move(val)) { }
  [[nodiscard]] bool isBool() const { return value.type() == Value::Type::BOOL; }
  [[nodiscard]] bool toBool() const { return value.toBool(); }
  [[nodiscard]] bool isDouble() const { return value.type() == Value::Type::NUMBER; }
  [[nodiscard]] double toDouble() const { return value.toDouble(); }
  [[nodiscard]] bool isString() const { return value.type() == Value::Type::STRING; }
  [[nodiscard]] const std::string& toString() const { return value.toStrUtf8Wrapper().toString(); }
  [[nodiscard]] bool isUndefined() const { return value.type() == Value::Type::UNDEFINED; }

  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  [[nodiscard]] bool isLiteral() const override { return true; }
private:
  const Value value;
};

class Range : public Expression
{
public:
  Range(Expression *begin, Expression *end, const Location& loc);
  Range(Expression *begin, Expression *step, Expression *end, const Location& loc);
  [[nodiscard]] const Expression *getBegin() const { return begin.get(); }
  [[nodiscard]] const Expression *getStep() const { return step.get(); }
  [[nodiscard]] const Expression *getEnd() const { return end.get(); }
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  [[nodiscard]] bool isLiteral() const override;
private:
  std::shared_ptr<Expression> begin;
  std::shared_ptr<Expression> step;
  std::shared_ptr<Expression> end;
};

class Vector : public Expression
{
public:
  Vector(const Location& loc);
  const std::vector<std::shared_ptr<Expression>>& getChildren() const { return children; }
  Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  void emplace_back(Expression *expr);
  bool isLiteral() const override;
private:
  std::vector<std::shared_ptr<Expression>> children;
  mutable boost::tribool literal_flag; // cache if already computed
};

class Lookup : public Expression
{
public:
  Lookup(std::string name, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  [[nodiscard]] const std::string& get_name() const { return name; }
private:
  std::string name;
};

class MemberLookup : public Expression
{
public:
  MemberLookup(Expression *expr, std::string member, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  std::shared_ptr<Expression> expr;
  std::string member;
};

class FunctionCall : public Expression
{
public:
  FunctionCall(Expression *expr, AssignmentList arglist, const Location& loc);
  [[nodiscard]] boost::optional<CallableFunction> evaluate_function_expression(const std::shared_ptr<const Context>& context) const;
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  [[nodiscard]] const std::string& get_name() const { return name; }
  static Expression *create(const std::string& funcname, const AssignmentList& arglist, Expression *expr, const Location& loc);
public:
  bool isLookup;
  std::string name;
  std::shared_ptr<Expression> expr;
  AssignmentList arguments;
};

class FunctionDefinition : public Expression
{
public:
  FunctionDefinition(Expression *expr, AssignmentList parameters, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
public:
  std::shared_ptr<const Context> context;
  AssignmentList parameters;
  std::shared_ptr<Expression> expr;
};

class Assert : public Expression
{
public:
  Assert(AssignmentList args, Expression *expr, const Location& loc);
  static void performAssert(const AssignmentList& arguments, const Location& location, const std::shared_ptr<const Context>& context);
  [[nodiscard]] const Expression *evaluateStep(const std::shared_ptr<const Context>& context) const;
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  AssignmentList arguments;
  std::shared_ptr<Expression> expr;
};

class Echo : public Expression
{
public:
  Echo(AssignmentList args, Expression *expr, const Location& loc);
  [[nodiscard]] const Expression *evaluateStep(const std::shared_ptr<const Context>& context) const;
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  AssignmentList arguments;
  std::shared_ptr<Expression> expr;
};

class Let : public Expression
{
public:
  Let(AssignmentList args, Expression *expr, const Location& loc);
  static void doSequentialAssignment(const AssignmentList& assignments, const Location& location, ContextHandle<Context>& targetContext);
  static ContextHandle<Context> sequentialAssignmentContext(const AssignmentList& assignments, const Location& location, const std::shared_ptr<const Context>& context);
  const Expression *evaluateStep(ContextHandle<Context>& targetContext) const;
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  AssignmentList arguments;
  std::shared_ptr<Expression> expr;
};

class ListComprehension : public Expression
{
public:
  ListComprehension(const Location& loc);
};

class LcIf : public ListComprehension
{
public:
  LcIf(Expression *cond, Expression *ifexpr, Expression *elseexpr, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  std::shared_ptr<Expression> cond;
  std::shared_ptr<Expression> ifexpr;
  std::shared_ptr<Expression> elseexpr;
};

class LcFor : public ListComprehension
{
public:
  LcFor(AssignmentList args, Expression *expr, const Location& loc);
  static void forEach(const AssignmentList& assignments, const Location& loc, const std::shared_ptr<const Context>& context, const std::function<void(const std::shared_ptr<const Context>&)>& operation, const std::function<void(size_t)>* pReserve = nullptr);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  AssignmentList arguments;
  std::shared_ptr<Expression> expr;
};

class LcForC : public ListComprehension
{
public:
  LcForC(AssignmentList args, AssignmentList incrargs, Expression *cond, Expression *expr, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  AssignmentList arguments;
  AssignmentList incr_arguments;
  std::shared_ptr<Expression> cond;
  std::shared_ptr<Expression> expr;
};

class LcEach : public ListComprehension
{
public:
  LcEach(Expression *expr, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  Value evalRecur(Value&& v, const std::shared_ptr<const Context>& context) const;
  std::shared_ptr<Expression> expr;
};

class LcLet : public ListComprehension
{
public:
  LcLet(AssignmentList args, Expression *expr, const Location& loc);
  [[nodiscard]] Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  AssignmentList arguments;
  std::shared_ptr<Expression> expr;
};

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
#include "core/Expression.h"

#include "utils/compiler_specific.h"
#include "core/Value.h"
#include <set>
#include <functional>
#include <ostream>
#include <cstdint>
#include <cmath>
#include <cassert>
#include <cstddef>
#include <memory>
#include <sstream>
#include <algorithm>
#include <typeinfo>
#include <utility>
#include <variant>
#include "utils/printutils.h"
#include "utils/StackCheck.h"
#include "core/Context.h"
#include "utils/exceptions.h"
#include "core/Parameters.h"
#include "utils/printutils.h"
#include "utils/boost-utils.h"
#include <boost/regex.hpp>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

Value Expression::checkUndef(Value&& val, const std::shared_ptr<const Context>& context) const {
  if (val.isUncheckedUndef()) LOG(message_group::Warning, loc, context->documentRoot(), "%1$s", val.toUndefString());
  return std::move(val);
}

bool Expression::isLiteral() const
{
  return false;
}

UnaryOp::UnaryOp(UnaryOp::Op op, Expression *expr, const Location& loc) : Expression(loc), op(op), expr(expr)
{
}

Value UnaryOp::evaluate(const std::shared_ptr<const Context>& context) const
{
  switch (this->op) {
  case (Op::Not):    return !this->expr->evaluate(context).toBool();
  case (Op::Negate): return checkUndef(-this->expr->evaluate(context), context);
  default:
    assert(false && "Non-existent unary operator!");
    throw EvaluationException("Non-existent unary operator!");
  }
}

const char *UnaryOp::opString() const
{
  switch (this->op) {
  case Op::Not:    return "!";
  case Op::Negate: return "-";
  default:
    assert(false && "Non-existent unary operator!");
    throw EvaluationException("Non-existent unary operator!");
  }
}

bool UnaryOp::isLiteral() const {
  return this->expr->isLiteral();
}

void UnaryOp::print(std::ostream& stream, const std::string&) const
{
  stream << opString() << *this->expr;
}

BinaryOp::BinaryOp(Expression *left, BinaryOp::Op op, Expression *right, const Location& loc) :
  Expression(loc), op(op), left(left), right(right)
{
}

Value BinaryOp::evaluate(const std::shared_ptr<const Context>& context) const
{
  switch (this->op) {
  case Op::LogicalAnd:
    return this->left->evaluate(context).toBool() && this->right->evaluate(context).toBool();
  case Op::LogicalOr:
    return this->left->evaluate(context).toBool() || this->right->evaluate(context).toBool();
  case Op::Exponent:
    return checkUndef(this->left->evaluate(context) ^ this->right->evaluate(context), context);
  case Op::Multiply:
    return checkUndef(this->left->evaluate(context) * this->right->evaluate(context), context);
  case Op::Divide:
    return checkUndef(this->left->evaluate(context) / this->right->evaluate(context), context);
  case Op::Modulo:
    return checkUndef(this->left->evaluate(context) % this->right->evaluate(context), context);
  case Op::Plus:
    return checkUndef(this->left->evaluate(context) + this->right->evaluate(context), context);
  case Op::Minus:
    return checkUndef(this->left->evaluate(context) - this->right->evaluate(context), context);
  case Op::Less:
    return checkUndef(this->left->evaluate(context) < this->right->evaluate(context), context);
  case Op::LessEqual:
    return checkUndef(this->left->evaluate(context) <= this->right->evaluate(context), context);
  case Op::Greater:
    return checkUndef(this->left->evaluate(context) > this->right->evaluate(context), context);
  case Op::GreaterEqual:
    return checkUndef(this->left->evaluate(context) >= this->right->evaluate(context), context);
  case Op::Equal:
    return checkUndef(this->left->evaluate(context) == this->right->evaluate(context), context);
  case Op::NotEqual:
    return checkUndef(this->left->evaluate(context) != this->right->evaluate(context), context);
  default:
    assert(false && "Non-existent binary operator!");
    throw EvaluationException("Non-existent binary operator!");
  }
}

const char *BinaryOp::opString() const
{
  switch (this->op) {
  case Op::LogicalAnd:   return "&&";
  case Op::LogicalOr:    return "||";
  case Op::Exponent:     return "^";
  case Op::Multiply:     return "*";
  case Op::Divide:       return "/";
  case Op::Modulo:       return "%";
  case Op::Plus:         return "+";
  case Op::Minus:        return "-";
  case Op::Less:         return "<";
  case Op::LessEqual:    return "<=";
  case Op::Greater:      return ">";
  case Op::GreaterEqual: return ">=";
  case Op::Equal:        return "==";
  case Op::NotEqual:     return "!=";
  default:
    assert(false && "Non-existent binary operator!");
    throw EvaluationException("Non-existent binary operator!");
  }
}

void BinaryOp::print(std::ostream& stream, const std::string&) const
{
  stream << "(" << *this->left << " " << opString() << " " << *this->right << ")";
}

TernaryOp::TernaryOp(Expression *cond, Expression *ifexpr, Expression *elseexpr, const Location& loc)
  : Expression(loc), cond(cond), ifexpr(ifexpr), elseexpr(elseexpr)
{
}

const Expression *TernaryOp::evaluateStep(const std::shared_ptr<const Context>& context) const
{
  return this->cond->evaluate(context).toBool() ? this->ifexpr.get() : this->elseexpr.get();
}

Value TernaryOp::evaluate(const std::shared_ptr<const Context>& context) const
{
  return evaluateStep(context)->evaluate(context);
}

void TernaryOp::print(std::ostream& stream, const std::string&) const
{
  stream << "(" << *this->cond << " ? " << *this->ifexpr << " : " << *this->elseexpr << ")";
}

ArrayLookup::ArrayLookup(Expression *array, Expression *index, const Location& loc)
  : Expression(loc), array(array), index(index)
{
}

Value ArrayLookup::evaluate(const std::shared_ptr<const Context>& context) const {
  return this->array->evaluate(context)[this->index->evaluate(context)];
}

void ArrayLookup::print(std::ostream& stream, const std::string&) const
{
  stream << *array << "[" << *index << "]";
}

Value Literal::evaluate(const std::shared_ptr<const Context>&) const
{
  return value.clone();
}

void Literal::print(std::ostream& stream, const std::string&) const
{
  stream << value;
}

Range::Range(Expression *begin, Expression *end, const Location& loc)
  : Expression(loc), begin(begin), end(end)
{
}

Range::Range(Expression *begin, Expression *step, Expression *end, const Location& loc)
  : Expression(loc), begin(begin), step(step), end(end)
{
}

/**
 * This is separated because both PRINT_DEPRECATION and PRINT use
 * quite a lot of stack space and the method using it evaluate()
 * is called often when recursive functions are evaluated.
 * noinline is required, as we here specifically optimize for stack usage
 * during normal operating, not runtime during error handling.
 */
static void NOINLINE print_range_depr(const Location& loc, const std::shared_ptr<const Context>& context){
  LOG(message_group::Deprecated, loc, context->documentRoot(), "Using ranges of the form [begin:end] with begin value greater than the end value is deprecated");
}

static void NOINLINE print_range_err(const std::string& begin, const std::string& step, const Location& loc, const std::shared_ptr<const Context>& context){
  LOG(message_group::Warning, loc, context->documentRoot(), "begin %1$s than the end, but step %2$s", begin, step);
}

Value Range::evaluate(const std::shared_ptr<const Context>& context) const
{
  Value beginValue = this->begin->evaluate(context);
  if (beginValue.type() == Value::Type::NUMBER) {
    Value endValue = this->end->evaluate(context);
    if (endValue.type() == Value::Type::NUMBER) {
      double begin_val = beginValue.toDouble();
      double end_val = endValue.toDouble();

      if (!this->step) {
        if (end_val < begin_val) {
          std::swap(begin_val, end_val);
          print_range_depr(loc, context);
        }
        return RangeType(begin_val, end_val);
      } else {
        Value stepValue = this->step->evaluate(context);
        if (stepValue.type() == Value::Type::NUMBER) {
          double step_val = stepValue.toDouble();
          if (this->isLiteral()) {
            if ((step_val > 0) && (end_val < begin_val)) {
              print_range_err("is greater", "is positive", loc, context);
            } else if ((step_val < 0) && (end_val > begin_val)) {
              print_range_err("is smaller", "is negative", loc, context);
            }
          }
          return RangeType(begin_val, step_val, end_val);
        }
      }
    }
  }
  return Value::undefined.clone();
}

void Range::print(std::ostream& stream, const std::string&) const
{
  stream << "[" << *this->begin;
  if (this->step) stream << " : " << *this->step;
  stream << " : " << *this->end;
  stream << "]";
}

bool Range::isLiteral() const {
  return this->step ?
         begin->isLiteral() && end->isLiteral() && step->isLiteral() :
         begin->isLiteral() && end->isLiteral();
}

Vector::Vector(const Location& loc) : Expression(loc), literal_flag(unknown)
{
}

bool Vector::isLiteral() const {
  if (unknown(literal_flag)) {
    for (const auto& e : this->children) {
      if (!e->isLiteral()) {
        literal_flag = false;
        return false;
      }
    }
    literal_flag = true;
    return true;
  } else {
    return bool(literal_flag);
  }
}

void Vector::emplace_back(Expression *expr)
{
  this->children.emplace_back(expr);
}

Value Vector::evaluate(const std::shared_ptr<const Context>& context) const
{
  if (children.size() == 1) {
    Value val = children.front()->evaluate(context);
    // If only 1 EmbeddedVectorType, convert to plain VectorType
    if (val.type() == Value::Type::EMBEDDED_VECTOR) {
      return VectorType(std::move(val.toEmbeddedVectorNonConst()));
    } else {
      VectorType vec(context->session());
      vec.emplace_back(std::move(val));
      return std::move(vec);
    }
  } else {
    VectorType vec(context->session());
    vec.reserve(this->children.size());
    for (const auto& e : this->children) vec.emplace_back(e->evaluate(context));
    return std::move(vec);
  }
}

void Vector::print(std::ostream& stream, const std::string&) const
{
  stream << "[";
  for (size_t i = 0; i < this->children.size(); ++i) {
    if (i > 0) stream << ", ";
    stream << *this->children[i];
  }
  stream << "]";
}

Lookup::Lookup(std::string name, const Location& loc) : Expression(loc), name(std::move(name))
{
}

Value Lookup::evaluate(const std::shared_ptr<const Context>& context) const
{
  return context->lookup_variable(this->name, loc).clone();
}

void Lookup::print(std::ostream& stream, const std::string&) const
{
  stream << this->name;
}

MemberLookup::MemberLookup(Expression *expr, std::string member, const Location& loc)
  : Expression(loc), expr(expr), member(std::move(member))
{
}

Value MemberLookup::evaluate(const std::shared_ptr<const Context>& context) const
{
  const Value& v = this->expr->evaluate(context);
  static const boost::regex re_swizzle_validation("^([xyzw]{1,4}|[rgba]{1,4})$");

  switch (v.type()) {
  case Value::Type::VECTOR:
    if (this->member.length() > 1 && boost::regex_match(this->member, re_swizzle_validation)) {
      VectorType ret(context->session());
      ret.reserve(this->member.length());
      for (const char& ch : this->member)
        switch (ch) {
        case 'r': case 'x': ret.emplace_back(v[0]); break;
        case 'g': case 'y': ret.emplace_back(v[1]); break;
        case 'b': case 'z': ret.emplace_back(v[2]); break;
        case 'a': case 'w': ret.emplace_back(v[3]); break;
        }
      return {std::move(ret)};
    }
    if (this->member == "x") return v[0];
    if (this->member == "y") return v[1];
    if (this->member == "z") return v[2];
    if (this->member == "w") return v[3];
    if (this->member == "r") return v[0];
    if (this->member == "g") return v[1];
    if (this->member == "b") return v[2];
    if (this->member == "a") return v[3];
    break;
  case Value::Type::RANGE:
    if (this->member == "begin") return v[0];
    if (this->member == "step") return v[1];
    if (this->member == "end") return v[2];
    break;
  case Value::Type::OBJECT:
    return v[this->member];
  default:
    break;
  }
  return Value::undefined.clone();
}

void MemberLookup::print(std::ostream& stream, const std::string&) const
{
  stream << *this->expr << "." << this->member;
}

FunctionDefinition::FunctionDefinition(Expression *expr, AssignmentList parameters, const Location& loc)
  : Expression(loc), context(nullptr), parameters(std::move(parameters)), expr(expr)
{
}

Value FunctionDefinition::evaluate(const std::shared_ptr<const Context>& context) const
{
  return FunctionPtr{FunctionType{context, expr, std::make_unique<AssignmentList>(parameters)}};
}

void FunctionDefinition::print(std::ostream& stream, const std::string& indent) const
{
  stream << indent << "function(";
  bool first = true;
  for (const auto& parameter : parameters) {
    stream << (first ? "" : ", ") << parameter->getName();
    if (parameter->getExpr()) {
      stream << " = " << *parameter->getExpr();
    }
    first = false;
  }
  stream << ") " << *this->expr;
}

/**
 * This is separated because PRINTB uses quite a lot of stack space
 * and the method using it evaluate()
 * is called often when recursive functions are evaluated.
 * noinline is required, as we here specifically optimize for stack usage
 * during normal operating, not runtime during error handling.
 */
static void NOINLINE print_err(const char *name, const Location& loc, const std::shared_ptr<const Context>& context){
  LOG(message_group::Error, loc, context->documentRoot(), "Recursion detected calling function '%1$s'", name);
}

/**
 * This is separated because PRINTB uses quite a lot of stack space
 * and the method using it evaluate()
 * is called often when recursive functions are evaluated.
 * noinline is required, as we here specifically optimize for stack usage
 * during normal operating, not runtime during error handling.
 */
static void NOINLINE print_trace(const FunctionCall *val, const std::shared_ptr<const Context>& context){
  LOG(message_group::Trace, val->location(), context->documentRoot(), "called by '%1$s'", val->get_name());
}

FunctionCall::FunctionCall(Expression *expr, AssignmentList args, const Location& loc)
  : Expression(loc), expr(expr), arguments(std::move(args))
{
  if (typeid(*expr) == typeid(Lookup)) {
    isLookup = true;
    const Lookup *lookup = static_cast<Lookup *>(expr);
    name = lookup->get_name();
  } else {
    isLookup = false;
    std::ostringstream s;
    s << "(";
    expr->print(s, "");
    s << ")";
    name = s.str();
  }
}

boost::optional<CallableFunction> FunctionCall::evaluate_function_expression(const std::shared_ptr<const Context>& context) const
{
  if (isLookup) {
    return context->lookup_function(name, location());
  } else {
    auto v = expr->evaluate(context);
    if (v.type() == Value::Type::FUNCTION) {
      return CallableFunction{std::move(v)};
    } else {
      LOG(message_group::Warning, loc, context->documentRoot(), "Can't call function on %1$s", v.typeName());
      return boost::none;
    }
  }
}

struct SimplifiedExpression {
  const Expression *expression;
  boost::optional<ContextHandle<Context>> new_context = boost::none;
  boost::optional<const FunctionCall *> new_active_function_call = boost::none;
};
using SimplificationResult = std::variant<SimplifiedExpression, Value>;

static SimplificationResult simplify_function_body(const Expression *expression, const std::shared_ptr<const Context>& context)
{
  if (!expression) {
    return Value::undefined.clone();
  } else {
    const auto& type = typeid(*expression);
    if (type == typeid(TernaryOp)) {
      const auto *ternary = static_cast<const TernaryOp *>(expression);
      return SimplifiedExpression{ternary->evaluateStep(context)};
    } else if (type == typeid(Assert)) {
      const auto *assertion = static_cast<const Assert *>(expression);
      return SimplifiedExpression{assertion->evaluateStep(context)};
    } else if (type == typeid(Echo)) {
      const Echo *echo = static_cast<const Echo *>(expression);
      return SimplifiedExpression{echo->evaluateStep(context)};
    } else if (type == typeid(Let)) {
      const Let *let = static_cast<const Let *>(expression);
      ContextHandle<Context> let_context{Context::create<Context>(context)};
      let_context->apply_config_variables(*context);
      return SimplifiedExpression{let->evaluateStep(let_context), std::move(let_context)};
    } else if (type == typeid(FunctionCall)) {
      const auto *call = static_cast<const FunctionCall *>(expression);

      const Expression *function_body;
      const AssignmentList *required_parameters;
      std::shared_ptr<const Context> defining_context;

      auto f = call->evaluate_function_expression(context);
      if (!f) {
        return Value::undefined.clone();
      } else {
        auto index = f->index();
        if (index == 0) {
          return std::get<const BuiltinFunction *>(*f)->evaluate(context, call);
        } else if (index == 1) {
          CallableUserFunction callable = std::get<CallableUserFunction>(*f);
          function_body = callable.function->expr.get();
          required_parameters = &callable.function->parameters;
          defining_context = callable.defining_context;
        } else {
          const FunctionType *function;
          if (index == 2) {
            function = &std::get<Value>(*f).toFunction();
          } else if (index == 3) {
            function = &std::get<const Value *>(*f)->toFunction();
          } else {
            assert(false);
          }
          function_body = function->getExpr().get();
          required_parameters = function->getParameters().get();
          defining_context = function->getContext();
        }
      }
      ContextHandle<Context> body_context{Context::create<Context>(defining_context)};
      body_context->apply_config_variables(*context);
      Arguments arguments{call->arguments, context};
      Parameters parameters = Parameters::parse(std::move(arguments), call->location(), *required_parameters, defining_context);
      body_context->apply_variables(std::move(parameters).to_context_frame());

      return SimplifiedExpression{function_body, std::move(body_context), call};
    } else {
      return expression->evaluate(context);
    }
  }
}

Value FunctionCall::evaluate(const std::shared_ptr<const Context>& context) const
{
  const auto& name = get_name();
  if (StackCheck::inst().check()) {
    print_err(name.c_str(), loc, context);
    throw RecursionException::create("function", name, this->loc);
  }

  // Repeatedly simplify expr until it reduces to either a tail call,
  // or an expression that cannot be simplified in-place. If the latter,
  // recurse. If the former, substitute the function body for expr,
  // thereby implementing tail recursion optimization.
  unsigned int recursion_depth = 0;
  const FunctionCall *current_call = this;

  ContextHandle<Context> expression_context{Context::create<Context>(context)};
  const Expression *expression = this;
  while (true) {
    try {
      auto result = simplify_function_body(expression, *expression_context);
      if (Value *value = std::get_if<Value>(&result)) {
        return std::move(*value);
      }

      SimplifiedExpression *simplified_expression = std::get_if<SimplifiedExpression>(&result);
      assert(simplified_expression);

      expression = simplified_expression->expression;
      if (simplified_expression->new_context) {
        expression_context = std::move(*simplified_expression->new_context);
      }
      if (simplified_expression->new_active_function_call) {
        current_call = *simplified_expression->new_active_function_call;
        if (recursion_depth++ == 1000000) {
          LOG(message_group::Error, expression->location(), expression_context->documentRoot(), "Recursion detected calling function '%1$s'", current_call->name);
          throw RecursionException::create("function", current_call->name, current_call->location());
        }
      }
    } catch (EvaluationException& e) {
      if (e.traceDepth > 0) {
        print_trace(current_call, *expression_context);
        e.traceDepth--;
      }
      throw;
    }
  }
}

void FunctionCall::print(std::ostream& stream, const std::string&) const
{
  stream << this->get_name() << "(" << this->arguments << ")";
}

Expression *FunctionCall::create(const std::string& funcname, const AssignmentList& arglist, Expression *expr, const Location& loc)
{
  if (funcname == "assert") {
    return new Assert(arglist, expr, loc);
  } else if (funcname == "echo") {
    return new Echo(arglist, expr, loc);
  } else if (funcname == "let") {
    return new Let(arglist, expr, loc);
  }
  return nullptr;
  // TODO: Generate error/warning if expr != 0?
  //return new FunctionCall(funcname, arglist, loc);
}

Assert::Assert(AssignmentList args, Expression *expr, const Location& loc)
  : Expression(loc), arguments(std::move(args)), expr(expr)
{

}

void Assert::performAssert(const AssignmentList& arguments, const Location& location, const std::shared_ptr<const Context>& context)
{
  Parameters parameters = Parameters::parse(Arguments(arguments, context), location, {"condition"}, {"message"});
  const Expression *conditionExpression = nullptr;
  for (const auto& argument : arguments) {
    if (argument->getName() == "" || argument->getName() == "condition") {
      conditionExpression = argument->getExpr().get();
      break;
    }
  }

  if (!parameters["condition"].toBool()) {
    std::string conditionString = conditionExpression ? STR(" '", *conditionExpression, "'") : "";
    std::string messageString = parameters.contains("message") ? (": " + parameters["message"].toEchoStringNoThrow()) : "";
    LOG(message_group::Error, location, context->documentRoot(), "Assertion%1$s failed%2$s", conditionString, messageString);
    throw AssertionFailedException("Assertion Failed", location);
  }
}

const Expression *Assert::evaluateStep(const std::shared_ptr<const Context>& context) const
{
  performAssert(this->arguments, this->loc, context);
  return expr.get();
}

Value Assert::evaluate(const std::shared_ptr<const Context>& context) const
{
  const Expression *nextexpr = evaluateStep(context);
  return nextexpr ? nextexpr->evaluate(context) : Value::undefined.clone();
}

void Assert::print(std::ostream& stream, const std::string&) const
{
  stream << "assert(" << this->arguments << ")";
  if (this->expr) stream << " " << *this->expr;
}

Echo::Echo(AssignmentList args, Expression *expr, const Location& loc)
  : Expression(loc), arguments(std::move(args)), expr(expr)
{

}

const Expression *Echo::evaluateStep(const std::shared_ptr<const Context>& context) const
{
  Arguments arguments{this->arguments, context};
  LOG(message_group::Echo, "%1$s", STR(arguments));
  return expr.get();
}

Value Echo::evaluate(const std::shared_ptr<const Context>& context) const
{
  const Expression *nextexpr = evaluateStep(context);
  return nextexpr ? nextexpr->evaluate(context) : Value::undefined.clone();
}

void Echo::print(std::ostream& stream, const std::string&) const
{
  stream << "echo(" << this->arguments << ")";
  if (this->expr) stream << " " << *this->expr;
}

Let::Let(AssignmentList args, Expression *expr, const Location& loc)
  : Expression(loc), arguments(std::move(args)), expr(expr)
{
}

void Let::doSequentialAssignment(const AssignmentList& assignments, const Location& location, ContextHandle<Context>& targetContext)
{
  std::set<std::string> seen;
  for (const auto& assignment : assignments) {
    Value value = assignment->getExpr()->evaluate(*targetContext);
    if (assignment->getName().empty()) {
      LOG(message_group::Warning, location, targetContext->documentRoot(), "Assignment without variable name %1$s", value.toEchoStringNoThrow());
    } else if (seen.find(assignment->getName()) != seen.end()) {
      LOG(message_group::Warning, location, targetContext->documentRoot(), "Ignoring duplicate variable assignment %1$s = %2$s", assignment->getName(), value.toEchoStringNoThrow());
    } else {
      targetContext->set_variable(assignment->getName(), std::move(value));
      seen.insert(assignment->getName());
    }
  }
}

ContextHandle<Context> Let::sequentialAssignmentContext(const AssignmentList& assignments, const Location& location, const std::shared_ptr<const Context>& context)
{
  ContextHandle<Context> letContext{Context::create<Context>(context)};
  doSequentialAssignment(assignments, location, letContext);
  return letContext;
}

const Expression *Let::evaluateStep(ContextHandle<Context>& targetContext) const
{
  doSequentialAssignment(this->arguments, this->location(), targetContext);
  return this->expr.get();
}

Value Let::evaluate(const std::shared_ptr<const Context>& context) const
{
  ContextHandle<Context> letContext{Context::create<Context>(context)};
  return evaluateStep(letContext)->evaluate(*letContext);
}

void Let::print(std::ostream& stream, const std::string&) const
{
  stream << "let(" << this->arguments << ") " << *expr;
}

ListComprehension::ListComprehension(const Location& loc) : Expression(loc)
{
}

LcIf::LcIf(Expression *cond, Expression *ifexpr, Expression *elseexpr, const Location& loc)
  : ListComprehension(loc), cond(cond), ifexpr(ifexpr), elseexpr(elseexpr)
{
}

Value LcIf::evaluate(const std::shared_ptr<const Context>& context) const
{
  const std::shared_ptr<Expression>& expr = this->cond->evaluate(context).toBool() ? this->ifexpr : this->elseexpr;
  if (expr) {
    return expr->evaluate(context);
  } else {
    return EmbeddedVectorType::Empty();
  }
}

void LcIf::print(std::ostream& stream, const std::string&) const
{
  stream << "if(" << *this->cond << ") (" << *this->ifexpr << ")";
  if (this->elseexpr) {
    stream << " else (" << *this->elseexpr << ")";
  }
}

LcEach::LcEach(Expression *expr, const Location& loc) : ListComprehension(loc), expr(expr)
{
}

// Need this for recurring into already embedded vectors, and performing "each" on their elements
//    Context is only passed along for the possible use in Range warning.
Value LcEach::evalRecur(Value&& v, const std::shared_ptr<const Context>& context) const
{
  if (v.type() == Value::Type::RANGE) {
    const RangeType& range = v.toRange();
    uint32_t steps = range.numValues();
    if (steps >= 1000000) {
      LOG(message_group::Warning, loc, context->documentRoot(), "Bad range parameter in for statement: too many elements (%1$lu)", steps);
    } else {
      EmbeddedVectorType vec(context->session());
      vec.reserve(range.numValues());
      for (double d : range) vec.emplace_back(d);
      return {std::move(vec)};
    }
  } else if (v.type() == Value::Type::VECTOR) {
    // Safe to move the overall vector ptr since we have a temporary value (could be a copy, or constructed just for us, doesn't matter)
    auto vec = EmbeddedVectorType(std::move(v.toVectorNonConst()));
    return {std::move(vec)};
  } else if (v.type() == Value::Type::EMBEDDED_VECTOR) {
    EmbeddedVectorType vec(context->session());
    vec.reserve(v.toEmbeddedVector().size());
    // Not safe to move values out of a vector, since it's shared_ptr maye be shared with another Value,
    // which should remain constant
    for (const auto& val : v.toEmbeddedVector()) vec.emplace_back(evalRecur(val.clone(), context) );
    return {std::move(vec)};
  } else if (v.type() == Value::Type::STRING) {
    EmbeddedVectorType vec(context->session());
    auto &wrapper = v.toStrUtf8Wrapper();
    vec.reserve(wrapper.size());
    for (auto ch : wrapper) vec.emplace_back(std::move(ch));
    return {std::move(vec)};
  } else if (v.type() != Value::Type::UNDEFINED) {
    return std::move(v);
  }
  return EmbeddedVectorType::Empty();
}

Value LcEach::evaluate(const std::shared_ptr<const Context>& context) const
{
  return evalRecur(this->expr->evaluate(context), context);
}

void LcEach::print(std::ostream& stream, const std::string&) const
{
  stream << "each (" << *this->expr << ")";
}

LcFor::LcFor(AssignmentList args, Expression *expr, const Location& loc)
  : ListComprehension(loc), arguments(std::move(args)), expr(expr)
{
}

static inline ContextHandle<Context> forContext(const std::shared_ptr<const Context>& context, const std::string& name, Value value)
{
  ContextHandle<Context> innerContext{Context::create<Context>(context)};
  innerContext->set_variable(name, std::move(value));
  return innerContext;
}

static void doForEach(
  const AssignmentList& assignments,
  const Location& location,
  const std::function<void(const std::shared_ptr<const Context>&)>& operation,
  size_t assignment_index,
  const std::shared_ptr<const Context>& context,
  const std::function<void(size_t)> *pReserve = nullptr
  ) {
  if (assignment_index >= assignments.size()) {
    operation(context);
    return;
  }

  const std::string& variable_name = assignments[assignment_index]->getName();
  Value variable_values = assignments[assignment_index]->getExpr()->evaluate(context);

  if (variable_values.type() == Value::Type::RANGE) {
    const RangeType& range = variable_values.toRange();
    uint32_t steps = range.numValues();
    if (steps >= 1000000) {
      LOG(message_group::Warning, location, context->documentRoot(),
          "Bad range parameter in for statement: too many elements (%1$lu)", steps);
    } else {
      if (pReserve) {
        (*pReserve)(steps);
      }
      for (double value : range) {
        doForEach(assignments, location, operation, assignment_index + 1,
                  *forContext(context, variable_name, value)
                  );
      }
    }
  } else if (variable_values.type() == Value::Type::VECTOR) {
    auto &vec = variable_values.toVector();
    if (pReserve) {
      (*pReserve)(vec.size());
    }
    for (const auto& value : vec) {
      doForEach(assignments, location, operation, assignment_index + 1,
                *forContext(context, variable_name, value.clone())
                );
    }
  } else if (variable_values.type() == Value::Type::OBJECT) {
    auto &keys = variable_values.toObject().keys();
    if (pReserve) {
      (*pReserve)(keys.size());
    }
    for (auto key : keys) {
      doForEach(assignments, location, operation, assignment_index + 1,
                *forContext(context, variable_name, key)
                );
    }
  } else if (variable_values.type() == Value::Type::STRING) {
    auto &wrapper = variable_values.toStrUtf8Wrapper();
    if (pReserve) {
      (*pReserve)(wrapper.size());
    }
    for (auto value : wrapper) {
      doForEach(assignments, location, operation, assignment_index + 1,
                *forContext(context, variable_name, Value(std::move(value)))
                );
    }
  } else if (variable_values.type() != Value::Type::UNDEFINED) {
    doForEach(assignments, location, operation, assignment_index + 1,
              *forContext(context, variable_name, std::move(variable_values))
              );
  }
}

void LcFor::forEach(const AssignmentList& assignments, const Location& loc, const std::shared_ptr<const Context>& context, const std::function<void(const std::shared_ptr<const Context>&)>& operation, const std::function<void(size_t)>* pReserve)
{
  doForEach(assignments, loc, operation, 0, context, pReserve);
}

Value LcFor::evaluate(const std::shared_ptr<const Context>& context) const
{
  EmbeddedVectorType vec(context->session());
  std::function<void(size_t)> reserve = [&vec](size_t capacity) {
    vec.reserve(capacity);
  };
  forEach(this->arguments, this->loc, context,
          [&vec, expression = expr.get()] (const std::shared_ptr<const Context>& iterationContext) {
    vec.emplace_back(expression->evaluate(iterationContext));
  }, &reserve);
  return {std::move(vec)};
}

void LcFor::print(std::ostream& stream, const std::string&) const
{
  stream << "for(" << this->arguments << ") (" << *this->expr << ")";
}

LcForC::LcForC(AssignmentList args, AssignmentList incrargs, Expression *cond, Expression *expr, const Location& loc)
  : ListComprehension(loc), arguments(std::move(args)), incr_arguments(std::move(incrargs)), cond(cond), expr(expr)
{
}

Value LcForC::evaluate(const std::shared_ptr<const Context>& context) const
{
  EmbeddedVectorType output(context->session());

  ContextHandle<Context> initialContext{Let::sequentialAssignmentContext(this->arguments, this->location(), context)};
  ContextHandle<Context> currentContext{Context::create<Context>(*initialContext)};

  unsigned int counter = 0;
  while (this->cond->evaluate(*currentContext).toBool()) {
    output.emplace_back(this->expr->evaluate(*currentContext));

    if (counter++ == 1000000) {
      LOG(message_group::Error, loc, context->documentRoot(), "For loop counter exceeded limit");
      throw LoopCntException::create("for", loc);
    }

    /*
     * The next context should be evaluated in the current context,
     * and replace the current context; but there is no reason for
     * it to _parent_ the current context, for the next context
     * replaces every variable in it. Keeping the next context
     * parented to the current context would keep the current in
     * memory unnecessarily, and greatly slow down variable lookup.
     * However, we can't just use apply_variables(), as this breaks
     * captured context references in lambda functions.
     * So, we reparent the next context to the initial context.
     */
    ContextHandle<Context> nextContext{Let::sequentialAssignmentContext(this->incr_arguments, this->location(), *currentContext)};
    currentContext = std::move(nextContext);
    currentContext->setParent(*initialContext);
  }
  return {std::move(output)};
}

void LcForC::print(std::ostream& stream, const std::string&) const
{
  stream
    << "for(" << this->arguments
    << ";" << *this->cond
    << ";" << this->incr_arguments
    << ") " << *this->expr;
}

LcLet::LcLet(AssignmentList args, Expression *expr, const Location& loc)
  : ListComprehension(loc), arguments(std::move(args)), expr(expr)
{
}

Value LcLet::evaluate(const std::shared_ptr<const Context>& context) const
{
  return this->expr->evaluate(*Let::sequentialAssignmentContext(this->arguments, this->location(), context));
}

void LcLet::print(std::ostream& stream, const std::string&) const
{
  stream << "let(" << this->arguments << ") (" << *this->expr << ")";
}

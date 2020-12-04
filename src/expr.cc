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
#include "compiler_specific.h"
#include "expression.h"
#include "value.h"
#include "evalcontext.h"
#include <cstdint>
#include <cmath>
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <typeinfo>
#include <forward_list>
#include "printutils.h"
#include "stackcheck.h"
#include "exceptions.h"
#include "feature.h"
#include "printutils.h"
#include <boost/bind.hpp>
#include "boost-utils.h"
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

// unnamed namespace
namespace {
	bool isListComprehension(const shared_ptr<Expression> &e) {
		return dynamic_cast<const ListComprehension *>(e.get());
	}

	void evaluate_sequential_assignment(const AssignmentList &assignment_list, std::shared_ptr<Context> context, const Location &loc) {
		ContextHandle<EvalContext> ctx{Context::create<EvalContext>(context, assignment_list, loc)};
		ctx->assignTo(context);
	}
}

namespace /* anonymous*/ {

	std::ostream &operator << (std::ostream &o, AssignmentList const& l) {
		for (size_t i=0; i < l.size(); ++i) {
			const auto &arg = l[i];
			if (i > 0) o << ", ";
			if (!arg->getName().empty()) o << arg->getName()  << " = ";
			o << *arg->getExpr();
		}
		return o;
	}

}

Value Expression::checkUndef(Value&& val, const std::shared_ptr<Context>& context) const {
	if (val.isUncheckedUndef())
		LOG(message_group::Warning,loc,context->documentPath(),"%1$s",val.toUndefString());
	return std::move(val);
}

bool Expression::isLiteral() const
{
    return false;
}

UnaryOp::UnaryOp(UnaryOp::Op op, Expression *expr, const Location &loc) : Expression(loc), op(op), expr(expr)
{
}

Value UnaryOp::evaluate(const std::shared_ptr<Context>& context) const
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

void UnaryOp::print(std::ostream &stream, const std::string &) const
{
	stream << opString() << *this->expr;
}

BinaryOp::BinaryOp(Expression *left, BinaryOp::Op op, Expression *right, const Location &loc) :
	Expression(loc), op(op), left(left), right(right)
{
}

Value BinaryOp::evaluate(const std::shared_ptr<Context>& context) const
{
	switch (this->op) {
	case Op::LogicalAnd:
		return this->left->evaluate(context).toBool() && this->right->evaluate(context).toBool();
	case Op::LogicalOr:
		return this->left->evaluate(context).toBool() || this->right->evaluate(context).toBool();
	case Op::Exponent:
		return checkUndef(this->left->evaluate(context) ^  this->right->evaluate(context), context);
	case Op::Multiply:
		return checkUndef(this->left->evaluate(context) *  this->right->evaluate(context), context);
	case Op::Divide:
		return checkUndef(this->left->evaluate(context) /  this->right->evaluate(context), context);
	case Op::Modulo:
		return checkUndef(this->left->evaluate(context) %  this->right->evaluate(context), context);
	case Op::Plus:
		return checkUndef(this->left->evaluate(context) +  this->right->evaluate(context), context);
	case Op::Minus:
		return checkUndef(this->left->evaluate(context) -  this->right->evaluate(context), context);
	case Op::Less:
		return checkUndef(this->left->evaluate(context) <  this->right->evaluate(context), context);
	case Op::LessEqual:
		return checkUndef(this->left->evaluate(context) <= this->right->evaluate(context), context);
	case Op::Greater:
		return checkUndef(this->left->evaluate(context) >  this->right->evaluate(context), context);
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

void BinaryOp::print(std::ostream &stream, const std::string &) const
{
	stream << "(" << *this->left << " " << opString() << " " << *this->right << ")";
}

TernaryOp::TernaryOp(Expression *cond, Expression *ifexpr, Expression *elseexpr, const Location &loc)
	: Expression(loc), cond(cond), ifexpr(ifexpr), elseexpr(elseexpr)
{
}

const shared_ptr<Expression>& TernaryOp::evaluateStep(const std::shared_ptr<Context>& context) const
{
	return this->cond->evaluate(context).toBool() ? this->ifexpr : this->elseexpr;
}

Value TernaryOp::evaluate(const std::shared_ptr<Context>& context) const
{
	return evaluateStep(context)->evaluate(context);
}

void TernaryOp::print(std::ostream &stream, const std::string &) const
{
	stream << "(" << *this->cond << " ? " << *this->ifexpr << " : " << *this->elseexpr << ")";
}

ArrayLookup::ArrayLookup(Expression *array, Expression *index, const Location &loc)
	: Expression(loc), array(array), index(index)
{
}

Value ArrayLookup::evaluate(const std::shared_ptr<Context>& context) const {
	return this->array->evaluate(context)[this->index->evaluate(context)];
}

void ArrayLookup::print(std::ostream &stream, const std::string &) const
{
	stream << *array << "[" << *index << "]";
}

Literal::Literal(Value val, const Location &loc) : Expression(loc), value(std::move(val))
{
}

Value Literal::evaluate(const std::shared_ptr<Context>&) const
{
	return this->value.clone();
}

void Literal::print(std::ostream &stream, const std::string &) const
{
    stream << this->value;
}

Range::Range(Expression *begin, Expression *end, const Location &loc)
	: Expression(loc), begin(begin), end(end)
{
}

Range::Range(Expression *begin, Expression *step, Expression *end, const Location &loc)
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
static void NOINLINE print_range_depr(const Location &loc, const std::shared_ptr<Context>& ctx){
	std::string locs = loc.toRelativeString(ctx->documentPath());
	LOG(message_group::Deprecated,loc,ctx->documentPath(),"Using ranges of the form [begin:end] with begin value greater than the end value is deprecated");
}

static void NOINLINE print_range_err(const std::string &begin, const std::string &step, const Location &loc, const std::shared_ptr<Context>& ctx){
	LOG(message_group::Warning,loc,ctx->documentPath(),"begin %1$s than the end, but step %2$s",begin,step);
}

Value Range::evaluate(const std::shared_ptr<Context>& context) const
{
	Value beginValue = this->begin->evaluate(context);
	if (beginValue.type() == Value::Type::NUMBER) {
		Value endValue = this->end->evaluate(context);
		if (endValue.type() == Value::Type::NUMBER) {
			double begin_val = beginValue.toDouble();
			double end_val   = endValue.toDouble();

			if (!this->step) {
				if (end_val < begin_val) {
					std::swap(begin_val,end_val);
					print_range_depr(loc, context);
				}
				return RangeType(begin_val, end_val);
			} else {
				Value stepValue = this->step->evaluate(context);
				if (stepValue.type() == Value::Type::NUMBER) {
					double step_val = stepValue.toDouble();
					if (this->isLiteral()) {
						if ((step_val>0) && (end_val < begin_val)) {
							print_range_err("is greater", "is positive", loc, context);
						}else if ((step_val<0) && (end_val > begin_val)) {
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

void Range::print(std::ostream &stream, const std::string &) const
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

Vector::Vector(const Location &loc) : Expression(loc), literal_flag(unknown)
{
}

bool Vector::isLiteral() const {
	if (unknown(literal_flag)) {
		for(const auto &e : this->children) {
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

Value Vector::evaluate(const std::shared_ptr<Context>& context) const
{
	if (children.size() == 1) {
		Value val = children.front()->evaluate(context);
		// If only 1 EmbeddedVectorType, convert to plain VectorType
		if (val.type() == Value::Type::EMBEDDED_VECTOR) {
			return VectorType(std::move(val.toEmbeddedVectorNonConst()));
		} else {
			VectorType vec;
			vec.emplace_back(std::move(val));
			return std::move(vec);
		}
	} else {
		VectorType vec;
		for(const auto &e : this->children) vec.emplace_back(e->evaluate(context));
		return std::move(vec);
	}
}

void Vector::print(std::ostream &stream, const std::string &) const
{
	stream << "[";
	for (size_t i=0; i < this->children.size(); ++i) {
		if (i > 0) stream << ", ";
		stream << *this->children[i];
	}
	stream << "]";
}

Lookup::Lookup(const std::string &name, const Location &loc) : Expression(loc), name(name)
{
}

Value Lookup::evaluate(const std::shared_ptr<Context>& context) const
{
	return context->lookup_variable(this->name,false,loc).clone();
}

const Value& Lookup::evaluateSilently(const std::shared_ptr<Context>& context) const
{
	return context->lookup_variable(this->name,true);
}

void Lookup::print(std::ostream &stream, const std::string &) const
{
	stream << this->name;
}

MemberLookup::MemberLookup(Expression *expr, const std::string &member, const Location &loc)
	: Expression(loc), expr(expr), member(member)
{
}

Value MemberLookup::evaluate(const std::shared_ptr<Context>& context) const
{
	const Value &v = this->expr->evaluate(context);

	if (v.type() == Value::Type::VECTOR) {
		if (this->member == "x") return v[0];
		if (this->member == "y") return v[1];
		if (this->member == "z") return v[2];
	} else if (v.type() == Value::Type::RANGE) {
		if (this->member == "begin") return v[0];
		if (this->member == "step") return v[1];
		if (this->member == "end") return v[2];
	}
	return Value::undefined.clone();
}

void MemberLookup::print(std::ostream &stream, const std::string &) const
{
	stream << *this->expr << "." << this->member;
}

FunctionDefinition::FunctionDefinition(Expression *expr, const AssignmentList &definition_arguments, const Location &loc)
	: Expression(loc), ctx(nullptr), definition_arguments(definition_arguments), expr(expr)
{
}

Value FunctionDefinition::evaluate(const std::shared_ptr<Context>& context) const
{
	return FunctionPtr{FunctionType{context, expr, std::unique_ptr<AssignmentList>{new AssignmentList{definition_arguments}}}};
}

void FunctionDefinition::print(std::ostream &stream, const std::string &indent) const
{
	stream << indent << "function(";
	bool first = true;
	for (const auto& assignment : definition_arguments) {
		stream << (first ? "" : ", ") << assignment->getName();
		if (assignment->getExpr()) {
			stream << " = " << *assignment->getExpr();
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
static void NOINLINE print_err(const char *name, const Location &loc, const std::shared_ptr<Context>& ctx){
	LOG(message_group::Error,loc,ctx->documentPath(),"Recursion detected calling function '%1$s'",name);
}

/**
 * This is separated because PRINTB uses quite a lot of stack space
 * and the method using it evaluate()
 * is called often when recursive functions are evaluated.
 * noinline is required, as we here specifically optimize for stack usage
 * during normal operating, not runtime during error handling.
*/
static void NOINLINE print_trace(const FunctionCall *val, const std::shared_ptr<Context>& ctx){
	LOG(message_group::Trace,val->location(),ctx->documentPath(),"called by '%1$s'",val->get_name());
}
static void NOINLINE print_invalid_function_call(const std::string& name, const std::shared_ptr<Context>& ctx, const Location& loc){
	LOG(message_group::Warning,loc,ctx->documentPath(),"Can't call function on %1$s",name);
}

FunctionCall::FunctionCall(Expression *expr, const AssignmentList &args, const Location &loc)
	: Expression(loc), expr(expr), arguments(args)
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

/**
 * Evaluates call parameters using context, and assigns the resulting values to tailCallContext.
 * As the name suggests, it's meant for basic tail recursion, where the function calls itself.
*/
void FunctionCall::prepareTailCallContext(const std::shared_ptr<Context> context, std::shared_ptr<Context> tailCallContext, const AssignmentList &definition_arguments)
{
	if (this->resolvedArguments.empty() && !this->arguments.empty()) {
		// Figure out parameter names
		ContextHandle<EvalContext> ec{Context::create<EvalContext>(context, this->arguments, this->loc)};
		this->resolvedArguments = ec->resolveArguments(definition_arguments, {}, false);
	}

	// FIXME: evaluate defaultArguments in FunctionDefinition / UserFunction and pass to FunctionCall instead of definition_arguments ?
	if (this->defaultArguments.empty() && !definition_arguments.empty()) {
		// Assign default values for unspecified parameters
		for (const auto &arg : definition_arguments) {
			if (this->resolvedArguments.find(arg->getName()) == this->resolvedArguments.end()) {
				this->defaultArguments.emplace_back(arg->getName(), arg->getExpr() ? arg->getExpr()->evaluate(context) : Value::undefined.clone());
			}
		}
	}

	// TODO REMOVE CLONES HERE?
	std::vector<std::pair<std::string, Value>> variables;
	variables.reserve(this->defaultArguments.size() + this->resolvedArguments.size());
	// Set default values for unspecified parameters
	for (const auto &arg : this->defaultArguments) {
		variables.emplace_back(arg.first, arg.second.clone());
	}
	// Set the given parameters
	for (const auto &arg : this->resolvedArguments) {
		variables.emplace_back(arg.first, arg.second->evaluate(context));
	}
	// Apply to tailCallContext
	for (const auto &var : variables) {
		tailCallContext->set_variable(var.first, var.second.clone());
	}
	// Apply config variables ($...)
	tailCallContext->apply_config_variables(context);
}

Value FunctionCall::evaluate(const std::shared_ptr<Context>& context) const
{
	const auto& name = get_name();
	if (StackCheck::inst().check()) {
		print_err(name.c_str(), loc, context);
		throw RecursionException::create("function", name, this->loc);
	}
	try {
		auto v = isLookup ? static_pointer_cast<Lookup>(expr)->evaluateSilently(context).clone() : expr->evaluate(context);
		ContextHandle<EvalContext> evalCtx{Context::create<EvalContext>(context, this->arguments, this->loc)};

		if (v.type() == Value::Type::FUNCTION) {
			if (name.size() > 0 && name.at(0) == '$') {
				print_invalid_function_call("dynamically scoped variable", context, loc);
				return Value::undefined.clone();
			} else {
				const auto &func = v.toFunction();
				return evaluate_function(name, func.getExpr(), *(func.getArgs()), func.getCtx(), evalCtx.ctx, this->loc);
			}
		} else if (isLookup) {
			return context->evaluate_function(name, evalCtx.ctx);
		} else {
			print_invalid_function_call(v.typeName(), context, loc);
			return Value::undefined.clone();
		}
	} catch (EvaluationException &e) {
		if (e.traceDepth > 0) {
			print_trace(this, context);
			e.traceDepth--;
		}
		throw;
	}
}

void FunctionCall::print(std::ostream &stream, const std::string &) const
{
	stream << this->get_name() << "(" << this->arguments << ")";
}

Expression * FunctionCall::create(const std::string &funcname, const AssignmentList &arglist, Expression *expr, const Location &loc)
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

Assert::Assert(const AssignmentList &args, Expression *expr, const Location &loc)
	: Expression(loc), arguments(args), expr(expr)
{

}

const shared_ptr<Expression>& Assert::evaluateStep(const std::shared_ptr<Context>& context) const
{
	ContextHandle<EvalContext> assert_context{Context::create<EvalContext>(context, this->arguments, this->loc)};
	ContextHandle<Context> c{Context::create<Context>(assert_context.ctx)};
	evaluate_assert(c.ctx, assert_context.ctx);
	return expr;
}

Value Assert::evaluate(const std::shared_ptr<Context>& context) const
{
	const shared_ptr<Expression>& nextexpr = evaluateStep(context);
	return nextexpr ? nextexpr->evaluate(context) : Value::undefined.clone();
}

void Assert::print(std::ostream &stream, const std::string &) const
{
	stream << "assert(" << this->arguments << ")";
	if (this->expr) stream << " " << *this->expr;
}

Echo::Echo(const AssignmentList &args, Expression *expr, const Location &loc)
	: Expression(loc), arguments(args), expr(expr)
{

}

const shared_ptr<Expression>& Echo::evaluateStep(const std::shared_ptr<Context>& context) const
{
	ContextHandle<EvalContext> echo_context{Context::create<EvalContext>(context, this->arguments, this->loc)};
	LOG(message_group::Echo,Location::NONE,"","%1$s",STR(*echo_context.ctx));
	return expr;
}

Value Echo::evaluate(const std::shared_ptr<Context>& context) const
{
	const shared_ptr<Expression>& nextexpr = evaluateStep(context);
	return nextexpr ? nextexpr->evaluate(context) : Value::undefined.clone();
}

void Echo::print(std::ostream &stream, const std::string &) const
{
	stream << "echo(" << this->arguments << ")";
	if (this->expr) stream << " " << *this->expr;
}

Let::Let(const AssignmentList &args, Expression *expr, const Location &loc)
	: Expression(loc), arguments(args), expr(expr)
{
}

const shared_ptr<Expression>& Let::evaluateStep(const std::shared_ptr<Context>& context) const
{
	evaluate_sequential_assignment(this->arguments, context, this->loc);
	return this->expr;
}

Value Let::evaluate(const std::shared_ptr<Context>& context) const
{
	ContextHandle<Context> c{Context::create<Context>(context)};
	const shared_ptr<Expression>& nextexpr = evaluateStep(c.ctx);
	return nextexpr->evaluate(c.ctx);
}

void Let::print(std::ostream &stream, const std::string &) const
{
	stream << "let(" << this->arguments << ") " << *expr;
}

ListComprehension::ListComprehension(const Location &loc) : Expression(loc)
{
}

LcIf::LcIf(Expression *cond, Expression *ifexpr, Expression *elseexpr, const Location &loc)
	: ListComprehension(loc), cond(cond), ifexpr(ifexpr), elseexpr(elseexpr)
{
}

Value LcIf::evaluate(const std::shared_ptr<Context>& context) const
{
	const shared_ptr<Expression> &expr = this->cond->evaluate(context).toBool() ? this->ifexpr : this->elseexpr;
	if (expr) {
		return expr->evaluate(context);
	} else {
		return EmbeddedVectorType::Empty();
	}
}

void LcIf::print(std::ostream &stream, const std::string &) const
{
	stream << "if(" << *this->cond << ") (" << *this->ifexpr << ")";
	if (this->elseexpr) {
		stream << " else (" << *this->elseexpr << ")";
	}
}

LcEach::LcEach(Expression *expr, const Location &loc) : ListComprehension(loc), expr(expr)
{
}

// Need this for recurring into already embedded vectors, and performing "each" on their elements
//    Context is only passed along for the possible use in Range warning.
Value LcEach::evalRecur(Value &&v, const std::shared_ptr<Context>& context) const
{
	if (v.type() == Value::Type::RANGE) {
		const RangeType &range = v.toRange();
		uint32_t steps = range.numValues();
		if (steps >= 1000000) {
           LOG(message_group::Warning,loc,context->documentPath(),"Bad range parameter in for statement: too many elements (%1$lu)",steps);
		} else {
			EmbeddedVectorType vec;
			for (double d : range) vec.emplace_back(d);
			return Value(std::move(vec));
		}
	} else if (v.type() == Value::Type::VECTOR) {
		// Safe to move the overall vector ptr since we have a temporary value (could be a copy, or constructed just for us, doesn't matter)
		auto vec = EmbeddedVectorType(std::move(v.toVectorNonConst()));
		return Value(std::move(vec));
	} else if (v.type() == Value::Type::EMBEDDED_VECTOR) {
		EmbeddedVectorType vec;
		// Not safe to move values out of a vector, since it's shared_ptr maye be shared with another Value,
		// which should remain constant
		for(const auto &val : v.toEmbeddedVector()) vec.emplace_back( evalRecur(val.clone(), context) );
		return Value(std::move(vec));
	} else if (v.type() == Value::Type::STRING) {
		EmbeddedVectorType vec;
		for (auto ch : v.toStrUtf8Wrapper()) vec.emplace_back(std::move(ch));
		return Value(std::move(vec));
	} else if (v.type() != Value::Type::UNDEFINED) {
		return std::move(v);
	}
	return EmbeddedVectorType::Empty();
}

Value LcEach::evaluate(const std::shared_ptr<Context>& context) const
{
	return evalRecur(this->expr->evaluate(context), context);
}

void LcEach::print(std::ostream &stream, const std::string &) const
{
	stream << "each (" << *this->expr << ")";
}

LcFor::LcFor(const AssignmentList &args, Expression *expr, const Location &loc)
	: ListComprehension(loc), arguments(args), expr(expr)
{
}

Value LcFor::evaluate(const std::shared_ptr<Context>& context) const
{
	ContextHandle<EvalContext> for_context{Context::create<EvalContext>(context, this->arguments, this->loc)};
	ContextHandle<Context> assign_context{Context::create<Context>(context)};
	ContextHandle<Context> c{Context::create<Context>(context)};

	// comprehension for statements are reduced by the parser to only contain one single element
	const std::string &it_name = for_context->getArgName(0);
	Value it_values = for_context->getArgValue(0, assign_context.ctx);

	if (it_values.type() == Value::Type::RANGE) {
		const RangeType &range = it_values.toRange();
		uint32_t steps = range.numValues();
		if (steps >= 1000000) {
           LOG(message_group::Warning,loc,context->documentPath(),"Bad range parameter in for statement: too many elements (%1$lu)",steps);
		} else {
			EmbeddedVectorType vec;
			for (double d : range) {
				c->set_variable(it_name, d);
				vec.emplace_back(this->expr->evaluate(c.ctx));
			}
			return Value(std::move(vec));
		}
	} else if (it_values.type() == Value::Type::VECTOR) {
		EmbeddedVectorType vec;
		for (const auto &el : it_values.toVector()) {
			c->set_variable(it_name, el.clone());
			vec.emplace_back(this->expr->evaluate(c.ctx));
		}
		return std::move(vec);
	} else if (it_values.type() == Value::Type::STRING) {
		EmbeddedVectorType vec;
		for (auto ch : it_values.toStrUtf8Wrapper()) {
			c->set_variable(it_name, std::move(ch));
			vec.emplace_back(this->expr->evaluate(c.ctx));
		}
		return Value(std::move(vec));
	} else if (it_values.type() != Value::Type::UNDEFINED) {
		c->set_variable(it_name, std::move(it_values));
		return this->expr->evaluate(c.ctx);
	}

	return EmbeddedVectorType::Empty();
}

void LcFor::print(std::ostream &stream, const std::string &) const
{
    stream << "for(" << this->arguments << ") (" << *this->expr << ")";
}

LcForC::LcForC(const AssignmentList &args, const AssignmentList &incrargs, Expression *cond, Expression *expr, const Location &loc)
	: ListComprehension(loc), arguments(args), incr_arguments(incrargs), cond(cond), expr(expr)
{
}

Value LcForC::evaluate(const std::shared_ptr<Context>& context) const
{
	EmbeddedVectorType vec;

    ContextHandle<Context> c{Context::create<Context>(context)};
    evaluate_sequential_assignment(this->arguments, c.ctx, this->loc);

	unsigned int counter = 0;
    while (this->cond->evaluate(c.ctx).toBool()) {
        vec.emplace_back(this->expr->evaluate(c.ctx));

        if (counter++ == 1000000) {
			LOG(message_group::Error,loc,context->documentPath(),"For loop counter exceeded limit");
            throw LoopCntException::create("for", loc);
        }

        ContextHandle<Context> tmp{Context::create<Context>(c.ctx)};
        evaluate_sequential_assignment(this->incr_arguments, tmp.ctx, this->loc);
        c->apply_variables(tmp.ctx);
    }
    return std::move(vec);
}

void LcForC::print(std::ostream &stream, const std::string &) const
{
    stream
        << "for(" << this->arguments
        << ";" << *this->cond
        << ";" << this->incr_arguments
        << ") " << *this->expr;
}

LcLet::LcLet(const AssignmentList &args, Expression *expr, const Location &loc)
	: ListComprehension(loc), arguments(args), expr(expr)
{
}

Value LcLet::evaluate(const std::shared_ptr<Context>& context) const
{
    ContextHandle<Context> c{Context::create<Context>(context)};
    evaluate_sequential_assignment(this->arguments, c.ctx, this->loc);
    return this->expr->evaluate(c.ctx);
}

void LcLet::print(std::ostream &stream, const std::string &) const
{
    stream << "let(" << this->arguments << ") (" << *this->expr << ")";
}

void evaluate_assert(const std::shared_ptr<Context>& context, const std::shared_ptr<EvalContext> evalctx)
{
	AssignmentList args;
	args += assignment("condition"), assignment("message");

	ContextHandle<Context> c{Context::create<Context>(context)};

	AssignmentMap assignments = evalctx->resolveArguments(args, {}, false);
	for (const auto &arg : args) {
		auto it = assignments.find(arg->getName());
		if (it != assignments.end()) {
			c->set_variable(arg->getName(), assignments[arg->getName()]->evaluate(evalctx));
		}
	}

	const Value &condition = c->lookup_variable("condition", false, evalctx->loc);

	if (!condition.toBool()) {
		const Expression *expr = assignments["condition"];
		const Value &message = c->lookup_variable("message", true);

		const auto exprText = expr ? STR(" '" << *expr << "'") : "";
		if (message.isDefined()) {
			LOG(message_group::Error,evalctx->loc,context->documentPath(),"Assertion%1$s failed: %2$s",exprText,message.toEchoString());
		} else {
			LOG(message_group::Error,evalctx->loc,context->documentPath(),"Assertion%1$s failed",exprText);
		}
		throw AssertionFailedException("Assertion Failed", evalctx->loc);
	}
}

Value evaluate_function(const std::string& name, const std::shared_ptr<Expression>& expr, const AssignmentList& definition_arguments,
		const std::shared_ptr<Context>& ctx, const std::shared_ptr<EvalContext>& evalctx, const Location& loc)
{
	if (!expr) return Value::undefined.clone();
	ContextHandle<Context> c_next{Context::create<Context>(ctx)}; // Context for next tail call
	c_next->setVariables(evalctx, definition_arguments);

	// Outer loop: to allow tail calls
	unsigned int counter = 0;
	while (true) {
		// Local contexts for a call. Nested contexts must be supported, to allow variable reassignment in an inner context.
		// I.e. "let(x=33) let(x=42) x" should evaluate to 42.
		// Cannot use std::vector, as it invalidates raw pointers.
		std::forward_list<ContextHandle<Context>> c_local_stack;
		c_local_stack.emplace_front(std::shared_ptr<Context>(new Context(c_next.ctx)));
		std::shared_ptr<Context> c_local = c_local_stack.front().ctx;

		// Inner loop: to follow a single execution path
		// Only 'break' inner loop for tail calls after calling prepareTailCallContext.
		// Otherwise continue looping over sub-expressions, or return subExpr->evaluate(...) directly if not applicable to tail calls.
		shared_ptr<Expression> subExpr = expr;
		while (true) {
			if (!subExpr) {
				return Value::undefined.clone();
			} else {
				const auto& subExprRef = *subExpr;
				if (typeid(subExprRef) == typeid(TernaryOp)) {
					const shared_ptr<TernaryOp> &ternary = static_pointer_cast<TernaryOp>(subExpr);
					subExpr = ternary->evaluateStep(c_local);
				}
				else if (typeid(subExprRef) == typeid(Assert)) {
					const shared_ptr<Assert> &assertion = static_pointer_cast<Assert>(subExpr);
					subExpr = assertion->evaluateStep(c_local);
				}
				else if (typeid(subExprRef) == typeid(Echo)) {
					const shared_ptr<Echo> &echo = static_pointer_cast<Echo>(subExpr);
					subExpr = echo->evaluateStep(c_local);
				}
				else if (typeid(subExprRef) == typeid(Let)) {
					const shared_ptr<Let> &let = static_pointer_cast<Let>(subExpr);
					// Start a new, nested context
					c_local_stack.emplace_front(std::shared_ptr<Context>(new Context(c_local)));
					c_local = c_local_stack.front().ctx;
					subExpr = let->evaluateStep(c_local);
				}
				else if (typeid(subExprRef) == typeid(FunctionCall)) {
					const shared_ptr<FunctionCall> &call = static_pointer_cast<FunctionCall>(subExpr);
					if (name == call->get_name()) {
						// Update c_next with new parameters for tail call
						call->prepareTailCallContext(c_local, c_next.ctx, definition_arguments);
						break;
					}
					else {
						return subExpr->evaluate(c_local);
					}
				}
				else {
					return subExpr->evaluate(c_local);
				}
			}
		}

		if (counter++ == 1000000) {
			LOG(message_group::Error,loc,ctx->documentPath(),"Recursion detected calling function '%1$s'",name);
			throw RecursionException::create("function", name,loc);
		}
	}

	return Value::undefined.clone();
}

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
#include "expression.h"
#include "value.h"
#include "evalcontext.h"
#include <cstdint>
#include <assert.h>
#include <sstream>
#include <algorithm>
#include "stl-utils.h"
#include "printutils.h"
#include "stackcheck.h"
#include "exceptions.h"
#include "feature.h"
#include <boost/bind.hpp>

#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

// unnamed namespace
namespace {
	Value::VectorType flatten(Value::VectorType const& vec) {
		int n = 0;
		for (unsigned int i = 0; i < vec.size(); i++) {
			assert(vec[i]->type() == Value::VECTOR);
			n += vec[i]->toVector().size();
		}
		Value::VectorType ret; ret.reserve(n);
		for (unsigned int i = 0; i < vec.size(); i++) {
			std::copy(vec[i]->toVector().begin(),vec[i]->toVector().end(),std::back_inserter(ret));
		}
		return ret;
	}

	void evaluate_sequential_assignment(const AssignmentList &assignment_list, Context &context) {
		EvalContext ctx(&context, assignment_list);
		ctx.assignTo(context);
	}
}

Expression::Expression() : first(NULL), second(NULL), third(NULL)
{
}

Expression::Expression(Expression *expr) : first(expr), second(NULL), third(NULL)
{
	children.push_back(expr);
}

Expression::Expression(Expression *left, Expression *right) : first(left), second(right), third(NULL)
{
	children.push_back(left);
	children.push_back(right);
}

Expression::Expression(Expression *expr1, Expression *expr2, Expression *expr3)
	: first(expr1), second(expr2), third(expr3)
{
	children.push_back(expr1);
	children.push_back(expr2);
	children.push_back(expr3);
}

Expression::~Expression()
{
	std::for_each(this->children.begin(), this->children.end(), del_fun<Expression>());
}

namespace /* anonymous*/ {

	std::ostream &operator << (std::ostream &o, AssignmentList const& l) {
		for (size_t i=0; i < l.size(); i++) {
			const Assignment &arg = l[i];
			if (i > 0) o << ", ";
			if (!arg.first.empty()) o << arg.first  << " = ";
			o << *arg.second;
		}
		return o;
	}

}

bool Expression::isListComprehension() const
{
	return false;
}

ExpressionNot::ExpressionNot(Expression *expr) : Expression(expr)
{
}

ValuePtr ExpressionNot::evaluate(const Context *context) const
{
	return !first->evaluate(context);
}

void ExpressionNot::print(std::ostream &stream) const
{
	stream << "!" << *first;
}

ExpressionLogicalAnd::ExpressionLogicalAnd(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionLogicalAnd::evaluate(const Context *context) const
{
	return this->first->evaluate(context) && this->second->evaluate(context);
}

void ExpressionLogicalAnd::print(std::ostream &stream) const
{
	stream << "(" << *first << " && " << *second << ")";
}

ExpressionLogicalOr::ExpressionLogicalOr(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionLogicalOr::evaluate(const Context *context) const
{
	return this->first->evaluate(context) || this->second->evaluate(context);
}

void ExpressionLogicalOr::print(std::ostream &stream) const
{
	stream << "(" << *first << " || " << *second << ")";
}

ExpressionMultiply::ExpressionMultiply(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionMultiply::evaluate(const Context *context) const
{
	return this->first->evaluate(context) * this->second->evaluate(context);
}

void ExpressionMultiply::print(std::ostream &stream) const
{
	stream << "(" << *first << " * " << *second << ")";
}

ExpressionDivision::ExpressionDivision(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionDivision::evaluate(const Context *context) const
{
	return this->first->evaluate(context) / this->second->evaluate(context);
}

void ExpressionDivision::print(std::ostream &stream) const
{
	stream << "(" << *first << " / " << *second << ")";
}

ExpressionModulo::ExpressionModulo(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionModulo::evaluate(const Context *context) const
{
	return this->first->evaluate(context) % this->second->evaluate(context);
}

void ExpressionModulo::print(std::ostream &stream) const
{
	stream << "(" << *first << " % " << *second << ")";
}

ExpressionPlus::ExpressionPlus(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionPlus::evaluate(const Context *context) const
{
	return this->first->evaluate(context) + this->second->evaluate(context);
}

void ExpressionPlus::print(std::ostream &stream) const
{
	stream << "(" << *first << " + " << *second << ")";
}

ExpressionMinus::ExpressionMinus(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionMinus::evaluate(const Context *context) const
{
	return this->first->evaluate(context) - this->second->evaluate(context);
}

void ExpressionMinus::print(std::ostream &stream) const
{
	stream << "(" << *first << " - " << *second << ")";
}

ExpressionLess::ExpressionLess(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionLess::evaluate(const Context *context) const
{
	return this->first->evaluate(context) < this->second->evaluate(context);
}

void ExpressionLess::print(std::ostream &stream) const
{
	stream << "(" << *first << " < " << *second << ")";
}

ExpressionLessOrEqual::ExpressionLessOrEqual(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionLessOrEqual::evaluate(const Context *context) const
{
	return this->first->evaluate(context) <= this->second->evaluate(context);
}

void ExpressionLessOrEqual::print(std::ostream &stream) const
{
	stream << "(" << *first << " <= " << *second << ")";
}

ExpressionEqual::ExpressionEqual(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionEqual::evaluate(const Context *context) const
{
	return this->first->evaluate(context) == this->second->evaluate(context);
}

void ExpressionEqual::print(std::ostream &stream) const
{
	stream << "(" << *first << " == " << *second << ")";
}

ExpressionNotEqual::ExpressionNotEqual(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionNotEqual::evaluate(const Context *context) const
{
	return this->first->evaluate(context) != this->second->evaluate(context);
}

void ExpressionNotEqual::print(std::ostream &stream) const
{
	stream << "(" << *first << " != " << *second << ")";
}

ExpressionGreaterOrEqual::ExpressionGreaterOrEqual(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionGreaterOrEqual::evaluate(const Context *context) const
{
	return this->first->evaluate(context) >= this->second->evaluate(context);
}

void ExpressionGreaterOrEqual::print(std::ostream &stream) const
{
	stream << "(" << *first << " >= " << *second << ")";
}

ExpressionGreater::ExpressionGreater(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionGreater::evaluate(const Context *context) const
{
	return this->first->evaluate(context) > this->second->evaluate(context);
}

void ExpressionGreater::print(std::ostream &stream) const
{
	stream << "(" << *first << " > " << *second << ")";
}

ExpressionTernary::ExpressionTernary(Expression *expr1, Expression *expr2, Expression *expr3) : Expression(expr1, expr2, expr3)
{
}

ValuePtr ExpressionTernary::evaluate(const Context *context) const
{
	return (this->first->evaluate(context) ? this->second : this->third)->evaluate(context);
}

void ExpressionTernary::print(std::ostream &stream) const
{
	stream << "(" << *first << " ? " << *second << " : " << *third << ")";
}

ExpressionArrayLookup::ExpressionArrayLookup(Expression *left, Expression *right) : Expression(left, right)
{
}

ValuePtr ExpressionArrayLookup::evaluate(const Context *context) const {
	return this->first->evaluate(context)[this->second->evaluate(context)];
}

void ExpressionArrayLookup::print(std::ostream &stream) const
{
	stream << *first << "[" << *second << "]";
}

ExpressionInvert::ExpressionInvert(Expression *expr) : Expression(expr)
{
}

ValuePtr ExpressionInvert::evaluate(const Context *context) const
{
	return -this->first->evaluate(context);
}

void ExpressionInvert::print(std::ostream &stream) const
{
	stream << "-" << *first;
}

ExpressionConst::ExpressionConst(const ValuePtr &val) : const_value(val)
{
}

ValuePtr ExpressionConst::evaluate(const class Context *) const
{
	return this->const_value;
}

void ExpressionConst::print(std::ostream &stream) const
{
    stream << *this->const_value;
}

ExpressionRange::ExpressionRange(Expression *expr1, Expression *expr2) : Expression(expr1, expr2)
{
}

ExpressionRange::ExpressionRange(Expression *expr1, Expression *expr2, Expression *expr3) : Expression(expr1, expr2, expr3)
{
}

ValuePtr ExpressionRange::evaluate(const Context *context) const
{
	ValuePtr v1 = this->first->evaluate(context);
	if (v1->type() == Value::NUMBER) {
		ValuePtr v2 = this->second->evaluate(context);
		if (v2->type() == Value::NUMBER) {
			if (this->children.size() == 2) {
				RangeType range(v1->toDouble(), v2->toDouble());
				return ValuePtr(range);
			} else {
				ValuePtr v3 = this->third->evaluate(context);
				if (v3->type() == Value::NUMBER) {
					RangeType range(v1->toDouble(), v2->toDouble(), v3->toDouble());
					return ValuePtr(range);
				}
			}
		}
	}
	return ValuePtr::undefined;
}

void ExpressionRange::print(std::ostream &stream) const
{
	stream << "[" << *first << " : " << *second;
	if (this->children.size() > 2) stream << " : " << *third;
	stream << "]";
}

ExpressionVector::ExpressionVector(Expression *expr) : Expression(expr)
{
}

ValuePtr ExpressionVector::evaluate(const Context *context) const
{
	Value::VectorType vec;
	for(const auto &e : this->children) {
		ValuePtr tmpval = e->evaluate(context);
		if (e->isListComprehension()) {
			const Value::VectorType result = tmpval->toVector();
			for (size_t i = 0;i < result.size();i++) {
				vec.push_back(result[i]);
			}
		} else {
			vec.push_back(tmpval);
		}
	}
	return ValuePtr(vec);
}

void ExpressionVector::print(std::ostream &stream) const
{
	stream << "[";
	for (size_t i=0; i < this->children.size(); i++) {
		if (i > 0) stream << ", ";
		stream << *this->children[i];
	}
	stream << "]";
}

ExpressionLookup::ExpressionLookup(const std::string &var_name) : var_name(var_name)
{
}

ValuePtr ExpressionLookup::evaluate(const Context *context) const
{
	return context->lookup_variable(this->var_name);
}

void ExpressionLookup::print(std::ostream &stream) const
{
	stream << this->var_name;
}

ExpressionMember::ExpressionMember(Expression *expr, const std::string &member)
	: Expression(expr), member(member)
{
}

ValuePtr ExpressionMember::evaluate(const Context *context) const
{
	ValuePtr v = this->first->evaluate(context);

	if (v->type() == Value::VECTOR) {
		if (this->member == "x") return v[0];
		if (this->member == "y") return v[1];
		if (this->member == "z") return v[2];
	} else if (v->type() == Value::RANGE) {
		if (this->member == "begin") return v[0];
		if (this->member == "step") return v[1];
		if (this->member == "end") return v[2];
	}
	return ValuePtr::undefined;
}

void ExpressionMember::print(std::ostream &stream) const
{
	stream << *first << "." << this->member;
}

ExpressionFunctionCall::ExpressionFunctionCall(const std::string &name, const AssignmentList &arglist, Expression *expr)
	: Expression(expr), funcname(name), call_arguments(arglist)
{
}

ExpressionFunctionCall * ExpressionFunctionCall::create(const std::string &name, const AssignmentList &arglist, Expression *expr)
{
	if (name == "echo") {
		return new ExpressionEcho(name, arglist, expr);
	} else if (name == "assert") {
		return new ExpressionAssert(name, arglist, expr);
	} else if (name == "let") {
		return new ExpressionLet(name, arglist, expr);
	} else {
		if (expr == 0) {
			return new ExpressionSimpleFunctionCall(name, arglist);
		} else {
			return new ExpressionError(name, arglist, expr);
		}
	}
}

void ExpressionFunctionCall::print(std::ostream &stream) const
{
	stream << this->funcname << "(" << this->call_arguments << ")";
	if (this->first) {
		stream << " " << *this->first;
	}
}

ExpressionSimpleFunctionCall::ExpressionSimpleFunctionCall(const std::string &name, const AssignmentList &arglist)
	: ExpressionFunctionCall(name, arglist, NULL)
{
}

ValuePtr ExpressionSimpleFunctionCall::evaluate(const Context *context) const
{
	if (StackCheck::inst()->check()) {
		throw RecursionException::create("function", funcname);
	}

	EvalContext c(context, this->call_arguments);
	ValuePtr result = context->evaluate_function(this->funcname, &c);

	return result;
}

ExpressionError::ExpressionError(const std::string &name, const AssignmentList &arglist, Expression *expr)
	: ExpressionFunctionCall(name, arglist, expr)
{
}

ValuePtr ExpressionError::evaluate(const Context * /*context*/) const
{
	throw RecursionException::create("error", funcname);
}

ExpressionEcho::ExpressionEcho(const std::string &name, const AssignmentList &arglist, Expression *expr)
	: ExpressionFunctionCall(name, arglist, expr)
{
}

ValuePtr ExpressionEcho::evaluate(const Context *context) const
{
	ExperimentalFeatureException::check(Feature::ExperimentalEchoExpression);

	EvalContext assignment_context(context, this->call_arguments);

	Context c(context);
	assignment_context.assignTo(c);

	ValuePtr result = this->first ? this->first->evaluate(&c) : ValuePtr::undefined;

	std::stringstream msg;
	EvalContext echo_context(&c, this->call_arguments);
	msg << "ECHO: " << echo_context;

	if (this->first) {
		if (echo_context.numArgs()) msg << ", ";
		msg << result->toEchoString();
	}

	PRINTB("%s", msg.str());

	return result;
}

ExpressionAssert::ExpressionAssert(const std::string &name, const AssignmentList &arglist, Expression *expr)
	: ExpressionFunctionCall(name, arglist, expr)
{
}

ValuePtr ExpressionAssert::evaluate(const Context *context) const
{
	EvalContext assert_context(context, this->call_arguments);

	Context c(&assert_context);
	evaluate_assert(c, &assert_context);

	ValuePtr result = this->first ? this->first->evaluate(&c) : ValuePtr::undefined;
	return result;
}

ExpressionLet::ExpressionLet(const std::string &name, const AssignmentList &arglist, Expression *expr)
	: ExpressionFunctionCall(name, arglist, expr)
{
}

ValuePtr ExpressionLet::evaluate(const Context *context) const
{
	Context c(context);
	evaluate_sequential_assignment(this->call_arguments, c);

	return this->first->evaluate(&c);
}

ExpressionLc::ExpressionLc(Expression *expr) : Expression(expr)
{
}

ExpressionLc::ExpressionLc(Expression *expr1, Expression *expr2)
    : Expression(expr1, expr2)
{
}

bool ExpressionLc::isListComprehension() const
{
	return true;
}

ExpressionLcIf::ExpressionLcIf(Expression *cond, Expression *exprIf, Expression *exprElse)
    : ExpressionLc(exprIf, exprElse), cond(cond)
{
}

ValuePtr ExpressionLcIf::evaluate(const Context *context) const
{
    if (this->second) {
    	ExperimentalFeatureException::check(Feature::ExperimentalElseExpression);
    }

    const Expression *expr = this->cond->evaluate(context) ? this->first : this->second;

	Value::VectorType vec;
    if (expr) {
        if (expr->isListComprehension()) {
            return expr->evaluate(context);
        } else {
           vec.push_back(expr->evaluate(context));
        }
    }

    return ValuePtr(vec);
}

void ExpressionLcIf::print(std::ostream &stream) const
{
    stream << "if(" << *this->cond << ") (" << *this->first << ")";
    if (this->second) {
        stream << " else (" << *this->second << ")";
    }
}

ExpressionLcEach::ExpressionLcEach(Expression *expr)
    : ExpressionLc(expr)
{
}

ValuePtr ExpressionLcEach::evaluate(const Context *context) const
{
	ExperimentalFeatureException::check(Feature::ExperimentalEachExpression);

	Value::VectorType vec;

    ValuePtr v = this->first->evaluate(context);

    if (v->type() == Value::RANGE) {
        RangeType range = v->toRange();
        uint32_t steps = range.numValues();
        if (steps >= 1000000) {
            PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu).", steps);
        } else {
            for (RangeType::iterator it = range.begin();it != range.end();it++) {
                vec.push_back(ValuePtr(*it));
            }
        }
    } else if (v->type() == Value::VECTOR) {
        Value::VectorType vector = v->toVector();
        for (size_t i = 0; i < v->toVector().size(); i++) {
            vec.push_back(vector[i]);
        }
    } else if (v->type() != Value::UNDEFINED) {
        vec.push_back(v);
    }

    if (this->first->isListComprehension()) {
        return ValuePtr(flatten(vec));
    } else {
        return ValuePtr(vec);
    }
}

void ExpressionLcEach::print(std::ostream &stream) const
{
    stream << "each (" << *this->first << ")";
}

ExpressionLcFor::ExpressionLcFor(const AssignmentList &arglist, Expression *expr)
    : ExpressionLc(expr), call_arguments(arglist)
{
}

ValuePtr ExpressionLcFor::evaluate(const Context *context) const
{
	Value::VectorType vec;

    EvalContext for_context(context, this->call_arguments);

    Context assign_context(context);

    // comprehension for statements are by the parser reduced to only contain one single element
    const std::string &it_name = for_context.getArgName(0);
    ValuePtr it_values = for_context.getArgValue(0, &assign_context);

    Context c(context);

    if (it_values->type() == Value::RANGE) {
        RangeType range = it_values->toRange();
        uint32_t steps = range.numValues();
        if (steps >= 1000000) {
            PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu).", steps);
        } else {
            for (RangeType::iterator it = range.begin();it != range.end();it++) {
                c.set_variable(it_name, ValuePtr(*it));
                vec.push_back(this->first->evaluate(&c));
            }
        }
    } else if (it_values->type() == Value::VECTOR) {
        for (size_t i = 0; i < it_values->toVector().size(); i++) {
            c.set_variable(it_name, it_values->toVector()[i]);
            vec.push_back(this->first->evaluate(&c));
        }
    } else if (it_values->type() != Value::UNDEFINED) {
        c.set_variable(it_name, it_values);
        vec.push_back(this->first->evaluate(&c));
    }

    if (this->first->isListComprehension()) {
        return ValuePtr(flatten(vec));
    } else {
        return ValuePtr(vec);
    }
}

void ExpressionLcFor::print(std::ostream &stream) const
{
    stream << "for(" << this->call_arguments << ") (" << *this->first << ")";
}

ExpressionLcForC::ExpressionLcForC(const AssignmentList &arglist, const AssignmentList &incrargs, Expression *cond, Expression *expr)
    : ExpressionLc(cond, expr), call_arguments(arglist), incr_arguments(incrargs)
{
}

ValuePtr ExpressionLcForC::evaluate(const Context *context) const
{
	ExperimentalFeatureException::check(Feature::ExperimentalForCExpression);

	Value::VectorType vec;

    Context c(context);
    evaluate_sequential_assignment(this->call_arguments, c);

	unsigned int counter = 0;
    while (this->first->evaluate(&c)) {
        vec.push_back(this->second->evaluate(&c));

		if (counter++ == 1000000) throw RecursionException::create("for loop", "");

        Context tmp(&c);
        evaluate_sequential_assignment(this->incr_arguments, tmp);
        c.apply_variables(tmp);
    }    

    if (this->second->isListComprehension()) {
        return ValuePtr(flatten(vec));
    } else {
        return ValuePtr(vec);
    }
}

void ExpressionLcForC::print(std::ostream &stream) const
{
    stream
        << "for(" << this->call_arguments
        << ";" << *this->first
        << ";" << this->incr_arguments
        << ") " << *this->second;
}

ExpressionLcLet::ExpressionLcLet(const AssignmentList &arglist, Expression *expr)
    : ExpressionLc(expr), call_arguments(arglist)
{
}

ValuePtr ExpressionLcLet::evaluate(const Context *context) const
{
    Context c(context);
	evaluate_sequential_assignment(this->call_arguments, c);

    return this->first->evaluate(&c);
}

void ExpressionLcLet::print(std::ostream &stream) const
{
    stream << "let(" << this->call_arguments << ") (" << *this->first << ")";
}

std::ostream &operator<<(std::ostream &stream, const Expression &expr)
{
	expr.print(stream);
	return stream;
}

void evaluate_assert(const Context &context, const class EvalContext *evalctx)
{
	ExperimentalFeatureException::check(Feature::ExperimentalAssertExpression);

	AssignmentList args;
	args += Assignment("condition"), Assignment("message");

	Context c(&context);
	const Context::Expressions expressions = c.setVariables(args, evalctx);
	const ValuePtr condition = c.lookup_variable("condition");

	if (!condition->toBool()) {
		std::stringstream msg;
		msg << "ERROR: Assertion";
		const Expression *expr = expressions.at("condition");
		if (expr) {
			msg << " '" << *expr << "'";
		}
		msg << " failed";
		const ValuePtr message = c.lookup_variable("message", true);
		if (message->isDefined()) {
			msg << ": " << message->toEchoString();
		}
		throw AssertionFailedException(msg.str());
	}
}

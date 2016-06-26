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

	void evaluate_sequential_assignment(const AssignmentList &assignment_list, Context *context) {
		EvalContext ctx(context, assignment_list);
		ctx.assignTo(*context);
	}
}

Expression::Expression()
{
}

Expression::~Expression()
{
}

namespace /* anonymous*/ {

	std::ostream &operator << (std::ostream &o, AssignmentList const& l) {
		for (size_t i=0; i < l.size(); i++) {
			const Assignment &arg = l[i];
			if (i > 0) o << ", ";
			if (!arg.name.empty()) o << arg.name  << " = ";
			o << *arg.expr;
		}
		return o;
	}

}

bool Expression::isListComprehension() const
{
	return false;
}

UnaryOp::UnaryOp(UnaryOp::Op op, Expression *expr) : op(op), expr(expr)
{
}

ValuePtr UnaryOp::evaluate(const Context *context) const
{
	switch (this->op) {
	case (Op::Not):
		return !this->expr->evaluate(context);
	case (Op::Negate):
		return -this->expr->evaluate(context);
	default:
		break;
		// FIXME: error:
	}
}

const char *UnaryOp::opString() const
{
	switch (this->op) {
	case Op::Not:
		return "!";
		break;
	case Op::Negate:
		return "-";
		break;
	default:
		break;
		// FIXME: Error: unknown op
	}
}

void UnaryOp::print(std::ostream &stream) const
{
	stream << opString() << *this->expr;
}

BinaryOp::BinaryOp(Expression *left, BinaryOp::Op op, Expression *right) :
	op(op), left(left), right(right)
{
}

ValuePtr BinaryOp::evaluate(const Context *context) const
{
	switch (this->op) {
	case Op::LogicalAnd:
		return this->left->evaluate(context) && this->right->evaluate(context);
		break;
	case Op::LogicalOr:
		return this->left->evaluate(context) || this->right->evaluate(context);
		break;
	case Op::Multiply:
		return this->left->evaluate(context) * this->right->evaluate(context);
		break;
	case Op::Divide:
		return this->left->evaluate(context) / this->right->evaluate(context);
		break;
	case Op::Modulo:
		return this->left->evaluate(context) % this->right->evaluate(context);
		break;
	case Op::Plus:
		return this->left->evaluate(context) + this->right->evaluate(context);
		break;
	case Op::Minus:
		return this->left->evaluate(context) - this->right->evaluate(context);
		break;
	case Op::Less:
		return this->left->evaluate(context) < this->right->evaluate(context);
		break;
	case Op::LessEqual:
		return this->left->evaluate(context) <= this->right->evaluate(context);
		break;
	case Op::Greater:
		return this->left->evaluate(context) > this->right->evaluate(context);
		break;
	case Op::GreaterEqual:
		return this->left->evaluate(context) >= this->right->evaluate(context);
		break;
	case Op::Equal:
		return this->left->evaluate(context) == this->right->evaluate(context);
		break;
	case Op::NotEqual:
		return this->left->evaluate(context) != this->right->evaluate(context);
		break;
	default:
		break;
		// FIXME: Error: unknown op
	}
}

const char *BinaryOp::opString() const
{
	switch (this->op) {
	case Op::LogicalAnd:
		return "&&";
		break;
	case Op::LogicalOr:
		return "||";
		break;
	case Op::Multiply:
		return "*";
		break;
	case Op::Divide:
		return "/";
		break;
	case Op::Modulo:
		return "%";
		break;
	case Op::Plus:
		return "+";
		break;
	case Op::Minus:
		return "-";
		break;
	case Op::Less:
		return "<";
		break;
	case Op::LessEqual:
		return "<=";
		break;
	case Op::Greater:
		return ">";
		break;
	case Op::GreaterEqual:
		return ">=";
		break;
	case Op::Equal:
		return "==";
		break;
	case Op::NotEqual:
		return "!=";
		break;
	default:
		break;
		// FIXME: Error: unknown op
	}
}

void BinaryOp::print(std::ostream &stream) const
{
	stream << "(" << *this->left << " " << opString() << " " << *this->right << ")";
}

TernaryOp::TernaryOp(Expression *cond, Expression *ifexpr, Expression *elseexpr)
	: cond(cond), ifexpr(ifexpr), elseexpr(elseexpr)
{
}

ValuePtr TernaryOp::evaluate(const Context *context) const
{
	return (this->cond->evaluate(context) ? this->ifexpr : this->elseexpr)->evaluate(context);
}

void TernaryOp::print(std::ostream &stream) const
{
	stream << "(" << *this->cond << " ? " << *this->ifexpr << " : " << *this->elseexpr << ")";
}

ArrayLookup::ArrayLookup(Expression *array, Expression *index)
	: array(array), index(index)
{
}

ValuePtr ArrayLookup::evaluate(const Context *context) const {
	return this->array->evaluate(context)[this->index->evaluate(context)];
}

void ArrayLookup::print(std::ostream &stream) const
{
	stream << *array << "[" << *index << "]";
}

Literal::Literal(const ValuePtr &val) : value(val)
{
}

ValuePtr Literal::evaluate(const class Context *) const
{
	return this->value;
}

void Literal::print(std::ostream &stream) const
{
    stream << *this->value;
}

Range::Range(Expression *begin, Expression *end)
	: begin(begin), end(end)
{
}

Range::Range(Expression *begin, Expression *step, Expression *end)
	: begin(begin), step(step), end(end)
{
}

ValuePtr Range::evaluate(const Context *context) const
{
	ValuePtr beginValue = this->begin->evaluate(context);
	if (beginValue->type() == Value::NUMBER) {
		ValuePtr endValue = this->end->evaluate(context);
		if (endValue->type() == Value::NUMBER) {
			if (!this->step) {
				RangeType range(beginValue->toDouble(), endValue->toDouble());
				return ValuePtr(range);
			} else {
				ValuePtr stepValue = this->step->evaluate(context);
				if (stepValue->type() == Value::NUMBER) {
					RangeType range(beginValue->toDouble(), stepValue->toDouble(), endValue->toDouble());
					return ValuePtr(range);
				}
			}
		}
	}
	return ValuePtr::undefined;
}

void Range::print(std::ostream &stream) const
{
	stream << "[" << *this->begin;
	if (this->step) stream << " : " << *this->step;
	stream << " : " << *this->end;
	stream << "]";
}

Vector::Vector()
{
}

void Vector::push_back(Expression *expr)
{
	this->children.push_back(shared_ptr<Expression>(expr));
}

ValuePtr Vector::evaluate(const Context *context) const
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

void Vector::print(std::ostream &stream) const
{
	stream << "[";
	for (size_t i=0; i < this->children.size(); i++) {
		if (i > 0) stream << ", ";
		stream << *this->children[i];
	}
	stream << "]";
}

Lookup::Lookup(const std::string &name) : name(name)
{
}

ValuePtr Lookup::evaluate(const Context *context) const
{
	return context->lookup_variable(this->name);
}

void Lookup::print(std::ostream &stream) const
{
	stream << this->name;
}

MemberLookup::MemberLookup(Expression *expr, const std::string &member)
	: expr(expr), member(member)
{
}

ValuePtr MemberLookup::evaluate(const Context *context) const
{
	ValuePtr v = this->expr->evaluate(context);

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

void MemberLookup::print(std::ostream &stream) const
{
	stream << *this->expr << "." << this->member;
}

FunctionCall::FunctionCall(const std::string &name, 
													 const AssignmentList &args)
	: name(name), arguments(args)
{
}

ValuePtr FunctionCall::evaluate(const Context *context) const
{
	if (StackCheck::inst()->check()) {
		throw RecursionException::create("function", this->name);
	}
    
	EvalContext c(context, this->arguments);
	ValuePtr result = context->evaluate_function(this->name, &c);

	return result;
}

void FunctionCall::print(std::ostream &stream) const
{
	stream << this->name << "(" << this->arguments << ")";
}

Let::Let(const AssignmentList &args, Expression *expr)
	: arguments(args), expr(expr)
{
}

ValuePtr Let::evaluate(const Context *context) const
{
	Context c(context);
	evaluate_sequential_assignment(this->arguments, &c);

	return this->expr->evaluate(&c);
}

void Let::print(std::ostream &stream) const
{
	stream << "let(" << this->arguments << ") " << *expr;
}

ListComprehension::ListComprehension()
{
}

bool ListComprehension::isListComprehension() const
{
	return true;
}

LcIf::LcIf(Expression *cond, Expression *ifexpr, Expression *elseexpr)
	: cond(cond), ifexpr(ifexpr), elseexpr(elseexpr)
{
}

ValuePtr LcIf::evaluate(const Context *context) const
{
    if (this->elseexpr) {
    	ExperimentalFeatureException::check(Feature::ExperimentalElseExpression);
    }

    const shared_ptr<Expression> &expr = this->cond->evaluate(context) ? this->ifexpr : this->elseexpr;
	
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

void LcIf::print(std::ostream &stream) const
{
    stream << "if(" << *this->cond << ") (" << *this->ifexpr << ")";
    if (this->elseexpr) {
        stream << " else (" << *this->elseexpr << ")";
    }
}

LcEach::LcEach(Expression *expr) : expr(expr)
{
}

ValuePtr LcEach::evaluate(const Context *context) const
{
	ExperimentalFeatureException::check(Feature::ExperimentalEachExpression);

	Value::VectorType vec;

    ValuePtr v = this->expr->evaluate(context);

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

    if (this->expr->isListComprehension()) {
        return ValuePtr(flatten(vec));
    } else {
        return ValuePtr(vec);
    }
}

void LcEach::print(std::ostream &stream) const
{
    stream << "each (" << *this->expr << ")";
}

LcFor::LcFor(const AssignmentList &args, Expression *expr)
	: arguments(args), expr(expr)
{
}

ValuePtr LcFor::evaluate(const Context *context) const
{
	Value::VectorType vec;

    EvalContext for_context(context, this->arguments);

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
                vec.push_back(this->expr->evaluate(&c));
            }
        }
    } else if (it_values->type() == Value::VECTOR) {
        for (size_t i = 0; i < it_values->toVector().size(); i++) {
            c.set_variable(it_name, it_values->toVector()[i]);
            vec.push_back(this->expr->evaluate(&c));
        }
    } else if (it_values->type() != Value::UNDEFINED) {
        c.set_variable(it_name, it_values);
        vec.push_back(this->expr->evaluate(&c));
    }

    if (this->expr->isListComprehension()) {
        return ValuePtr(flatten(vec));
    } else {
        return ValuePtr(vec);
    }
}

void LcFor::print(std::ostream &stream) const
{
    stream << "for(" << this->arguments << ") (" << *this->expr << ")";
}

LcForC::LcForC(const AssignmentList &args, const AssignmentList &incrargs, Expression *cond, Expression *expr)
	: arguments(args), incr_arguments(incrargs), cond(cond), expr(expr)
{
}

ValuePtr LcForC::evaluate(const Context *context) const
{
	ExperimentalFeatureException::check(Feature::ExperimentalForCExpression);

	Value::VectorType vec;

    Context c(context);
    evaluate_sequential_assignment(this->arguments, &c);

	unsigned int counter = 0;
    while (this->cond->evaluate(&c)) {
        vec.push_back(this->expr->evaluate(&c));

		if (counter++ == 1000000) throw RecursionException::create("for loop", "");

        Context tmp(&c);
        evaluate_sequential_assignment(this->incr_arguments, &tmp);
        c.apply_variables(tmp);
    }    

    if (this->expr->isListComprehension()) {
        return ValuePtr(flatten(vec));
    } else {
        return ValuePtr(vec);
    }
}

void LcForC::print(std::ostream &stream) const
{
    stream
        << "for(" << this->arguments
        << ";" << *this->cond
        << ";" << this->incr_arguments
        << ") " << *this->expr;
}

LcLet::LcLet(const AssignmentList &args, Expression *expr)
	: arguments(args), expr(expr)
{
}

ValuePtr LcLet::evaluate(const Context *context) const
{
    Context c(context);
    evaluate_sequential_assignment(this->arguments, &c);
    return this->expr->evaluate(&c);
}

void LcLet::print(std::ostream &stream) const
{
    stream << "let(" << this->arguments << ") (" << *this->expr << ")";
}

std::ostream &operator<<(std::ostream &stream, const Expression &expr)
{
	expr.print(stream);
	return stream;
}

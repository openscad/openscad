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
#include <assert.h>
#include <sstream>
#include <algorithm>
#include "stl-utils.h"
#include "printutils.h"
#include "stackcheck.h"
#include "exceptions.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

// unnamed namespace
namespace {
	Value::VectorType flatten(Value::VectorType const& vec) {
		int n = 0;
		for (unsigned int i = 0; i < vec.size(); i++) {
			assert(vec[i].type() == Value::VECTOR);
			n += vec[i].toVector().size();
		}
		Value::VectorType ret; ret.reserve(n);
		for (unsigned int i = 0; i < vec.size(); i++) {
			std::copy(vec[i].toVector().begin(),vec[i].toVector().end(),std::back_inserter(ret));
		}
		return ret;
	}

	void evaluate_sequential_assignment(const AssignmentList & assignment_list, Context *context) {
		EvalContext let_context(context, assignment_list);

		const bool allow_reassignment = false;

		for (unsigned int i = 0; i < let_context.numArgs(); i++) {
			if (!allow_reassignment && context->has_local_variable(let_context.getArgName(i))) {
				PRINTB("WARNING: Ignoring duplicate variable assignment %s = %s", let_context.getArgName(i) % let_context.getArgValue(i, context)->toString());
			} else {
				// NOTE: iteratively evaluated list of arguments
				context->set_variable(let_context.getArgName(i), let_context.getArgValue(i, context));
			}
		}
	}
}

Expression::Expression()
{
}

Expression::Expression(const ValuePtr &val)
{
	const_value = val;
}

Expression::Expression(const std::string &val)
{
	var_name = val;
}

Expression::Expression(const std::string &val, Expression *expr)
{
	var_name = val;
	children.push_back(expr);
	first = expr;
}

Expression::Expression(Expression *expr)
{
	children.push_back(expr);
	first = expr;
}

Expression::Expression(Expression *left, Expression *right)
{
	children.push_back(left);
	children.push_back(right);
	first = left;
	second = right;
}

Expression::Expression(Expression *expr1, Expression *expr2, Expression *expr3)
{
	children.push_back(expr1);
	children.push_back(expr2);
	children.push_back(expr3);
	first = expr1;
	second = expr2;
	third = expr3;
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

ExpressionConst::ExpressionConst(const ValuePtr &val) : Expression(val)
{
}

ValuePtr ExpressionConst::evaluate(const class Context *) const
{
	return ValuePtr(this->const_value);
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
				Value::RangeType range(v1->toDouble(), v2->toDouble());
				return ValuePtr(range);
			} else {
				ValuePtr v3 = this->third->evaluate(context);
				if (v3->type() == Value::NUMBER) {
					Value::RangeType range(v1->toDouble(), v2->toDouble(), v3->toDouble());
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
	BOOST_FOREACH(const Expression *e, this->children) {
		vec.push_back(*(e->evaluate(context)));
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

ExpressionLookup::ExpressionLookup(const std::string &val) : Expression(val)
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

ExpressionMember::ExpressionMember(const std::string &val, Expression *expr) : Expression(val, expr)
{
}

ValuePtr ExpressionMember::evaluate(const Context *context) const
{
	ValuePtr v = this->first->evaluate(context);

	if (v->type() == Value::VECTOR) {
		if (this->var_name == "x") return v[0];
		if (this->var_name == "y") return v[1];
		if (this->var_name == "z") return v[2];
	} else if (v->type() == Value::RANGE) {
		if (this->var_name == "begin") return v[0];
		if (this->var_name == "step") return v[1];
		if (this->var_name == "end") return v[2];
	}
	return ValuePtr::undefined;
}

void ExpressionMember::print(std::ostream &stream) const
{
	stream << *first << "." << this->var_name;
}

ExpressionFunction::ExpressionFunction()
{
}

ValuePtr ExpressionFunction::evaluate(const Context *context) const
{
	if (StackCheck::inst()->check()) {
		throw RecursionException("function", call_funcname);
	}
    
	EvalContext c(context, this->call_arguments);
	ValuePtr result = context->evaluate_function(this->call_funcname, &c);

	return result;
}

void ExpressionFunction::print(std::ostream &stream) const
{
	stream << this->call_funcname << "(" << this->call_arguments << ")";
}

ExpressionLet::ExpressionLet(Expression *expr) : Expression(expr)
{
}

ValuePtr ExpressionLet::evaluate(const Context *context) const
{
	Context c(context);
	evaluate_sequential_assignment(this->call_arguments, &c);

	return this->children[0]->evaluate(&c);
}

void ExpressionLet::print(std::ostream &stream) const
{
	stream << "let(" << this->call_arguments << ") " << *first;
}

ExpressionLcExpression::ExpressionLcExpression(Expression *expr) : Expression(expr)
{
}

ValuePtr ExpressionLcExpression::evaluate(const Context *context) const
{
	return this->children[0]->evaluate(context);
}

void ExpressionLcExpression::print(std::ostream &stream) const
{
	Expression const* c = this->first;
	
	stream << "[";
	
	do {
		if (c->call_funcname == "for") {
			stream << "for(" << c->call_arguments << ") ";
			c = c->children[0];
		} else if (c->call_funcname == "if") {
			stream << "if(" << c->children[0] << ") ";
			c = c->children[1];
		} else if (c->call_funcname == "let") {
			stream << "let(" << c->call_arguments << ") ";
			c = c->children[0];
		} else {
			assert(false && "Illegal list comprehension element");
		}
	} while (c->isListComprehension());
	
	stream << *c << "]";
}

ExpressionLc::ExpressionLc(Expression *expr) : Expression(expr)
{
}

ExpressionLc::ExpressionLc(Expression *expr1, Expression *expr2) : Expression(expr1, expr2)
{
}

bool ExpressionLc::isListComprehension() const
{
	return true;
}

ValuePtr ExpressionLc::evaluate(const Context *context) const
{
	Value::VectorType vec;

	if (this->call_funcname == "if") {
		if (this->children[0]->evaluate(context)) {
			if (this->children[1]->isListComprehension()) {
				return this->children[1]->evaluate(context);
			} else {
				vec.push_back((*this->children[1]->evaluate(context)));
			}
		}
		return ValuePtr(vec);
	} else if (this->call_funcname == "for") {
		EvalContext for_context(context, this->call_arguments);

		Context assign_context(context);

		// comprehension for statements are by the parser reduced to only contain one single element
		const std::string &it_name = for_context.getArgName(0);
		ValuePtr it_values = for_context.getArgValue(0, &assign_context);

		Context c(context);

		if (it_values->type() == Value::RANGE) {
			Value::RangeType range = it_values->toRange();
			boost::uint32_t steps = range.nbsteps();
			if (steps >= 1000000) {
				PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu).", steps);
			} else {
				for (Value::RangeType::iterator it = range.begin();it != range.end();it++) {
					c.set_variable(it_name, ValuePtr(*it));
					vec.push_back((*this->children[0]->evaluate(&c)));
				}
			}
		}
		else if (it_values->type() == Value::VECTOR) {
			for (size_t i = 0; i < it_values->toVector().size(); i++) {
				c.set_variable(it_name, it_values->toVector()[i]);
				vec.push_back((*this->children[0]->evaluate(&c)));
			}
		}
		else if (it_values->type() != Value::UNDEFINED) {
			c.set_variable(it_name, it_values);
			vec.push_back((*this->children[0]->evaluate(&c)));
		}
		if (this->children[0]->isListComprehension()) {
			return ValuePtr(flatten(vec));
		} else {
			return ValuePtr(vec);
		}
	} else if (this->call_funcname == "let") {
		Context c(context);
		evaluate_sequential_assignment(this->call_arguments, &c);

		return this->children[0]->evaluate(&c);
	} else {
		abort();
	}
}

void ExpressionLc::print(std::ostream &stream) const
{
	// FIXME: Implement?
}

std::ostream &operator<<(std::ostream &stream, const Expression &expr)
{
	expr.print(stream);
	return stream;
}

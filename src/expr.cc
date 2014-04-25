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
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

Expression::Expression() : recursioncount(0)
{
}

Expression::Expression(const std::string &type, 
											 Expression *left, Expression *right)
	: type(type), recursioncount(0)
{
	this->children.push_back(left);
	this->children.push_back(right);
}

Expression::Expression(const std::string &type, Expression *expr)
	: type(type), recursioncount(0)
{
	this->children.push_back(expr);
}

Expression::Expression(const Value &val) : const_value(val), type("C"), recursioncount(0)
{
}

Expression::~Expression()
{
	std::for_each(this->children.begin(), this->children.end(), del_fun<Expression>());
}

class FuncRecursionGuard
{
public:
	FuncRecursionGuard(const Expression &e) : expr(e) { 
		expr.recursioncount++; 
	}
	~FuncRecursionGuard() { expr.recursioncount--; }
	bool recursion_detected() const { return (expr.recursioncount > 1000); }
private:
	const Expression &expr;
};

// unnamed namespace
namespace {
	Value::VectorType flatten(Value::VectorType const& vec) {
		int n = 0;
		for (int i = 0; i < vec.size(); i++) {
			assert(vec[i].type() == Value::VECTOR);
			n += vec[i].toVector().size();
		}
		Value::VectorType ret; ret.reserve(n);
		for (int i = 0; i < vec.size(); i++) {
			std::copy(vec[i].toVector().begin(),vec[i].toVector().end(),std::back_inserter(ret));
		}
		return ret;
	}

	void evaluate_sequential_assignment(const AssignmentList & assignment_list, Context *context) {
		EvalContext let_context(context, assignment_list);

		for (int i = 0; i < let_context.numArgs(); i++) {
			// NOTE: iteratively evaluated list of arguments
			context->set_variable(let_context.getArgName(i), let_context.getArgValue(i, context));
		}
	}
}

Value Expression::evaluate_list_comprehension(const Context *context) const
{
	Value::VectorType vec;

	if (this->call_funcname == "if") {
		if (this->children[0]->evaluate(context)) {
			vec.push_back(this->children[1]->evaluate(context));
		}
		return vec;
	} else if (this->call_funcname == "for") {
		EvalContext for_context(context, this->call_arguments);

		Context assign_context(context);

		// comprehension for statements are by the parser reduced to only contain one single element
		const std::string &it_name = for_context.getArgName(0);
		const Value &it_values = for_context.getArgValue(0, &assign_context);

		Context c(context);

		if (it_values.type() == Value::RANGE) {
			Value::RangeType range = it_values.toRange();
			boost::uint32_t steps = range.nbsteps();
			if (steps >= 1000000) {
				PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu).", steps);
			} else {
				for (Value::RangeType::iterator it = range.begin();it != range.end();it++) {
					c.set_variable(it_name, Value(*it));
					vec.push_back(this->children[0]->evaluate(&c));
				}
			}
		}
		else if (it_values.type() == Value::VECTOR) {
			for (size_t i = 0; i < it_values.toVector().size(); i++) {
				c.set_variable(it_name, it_values.toVector()[i]);
				vec.push_back(this->children[0]->evaluate(&c));
			}
		}
		else if (it_values.type() != Value::UNDEFINED) {
			c.set_variable(it_name, it_values);
			vec.push_back(this->children[0]->evaluate(&c));
		}
		if (this->children[0]->type == "c") {
			return flatten(vec);
		} else {
			return vec;
		}
	} else if (this->call_funcname == "let") {
		Context c(context);
		evaluate_sequential_assignment(this->call_arguments, &c);

		return this->children[0]->evaluate(&c);
	} else {
		abort();
	}
}


Value Expression::evaluate(const Context *context) const
{
	if (this->type == "!")
		return ! this->children[0]->evaluate(context);
	if (this->type == "&&")
		return this->children[0]->evaluate(context) && this->children[1]->evaluate(context);
	if (this->type == "||")
		return this->children[0]->evaluate(context) || this->children[1]->evaluate(context);
	if (this->type == "*")
		return this->children[0]->evaluate(context) * this->children[1]->evaluate(context);
	if (this->type == "/")
		return this->children[0]->evaluate(context) / this->children[1]->evaluate(context);
	if (this->type == "%")
		return this->children[0]->evaluate(context) % this->children[1]->evaluate(context);
	if (this->type == "+")
		return this->children[0]->evaluate(context) + this->children[1]->evaluate(context);
	if (this->type == "-")
		return this->children[0]->evaluate(context) - this->children[1]->evaluate(context);
	if (this->type == "<")
		return this->children[0]->evaluate(context) < this->children[1]->evaluate(context);
	if (this->type == "<=")
		return this->children[0]->evaluate(context) <= this->children[1]->evaluate(context);
	if (this->type == "==")
		return this->children[0]->evaluate(context) == this->children[1]->evaluate(context);
	if (this->type == "!=")
		return this->children[0]->evaluate(context) != this->children[1]->evaluate(context);
	if (this->type == ">=")
		return this->children[0]->evaluate(context) >= this->children[1]->evaluate(context);
	if (this->type == ">")
		return this->children[0]->evaluate(context) > this->children[1]->evaluate(context);
	if (this->type == "?:") {
		return this->children[this->children[0]->evaluate(context) ? 1 : 2]->evaluate(context);
	}
	if (this->type == "[]") {
		return this->children[0]->evaluate(context)[this->children[1]->evaluate(context)];
	}
	if (this->type == "I")
		return -this->children[0]->evaluate(context);
	if (this->type == "C")
		return this->const_value;
	if (this->type == "R") {
		Value v1 = this->children[0]->evaluate(context);
		Value v2 = this->children[1]->evaluate(context);
                if (this->children.size() == 2) {
                        if (v1.type() == Value::NUMBER && v2.type() == Value::NUMBER) {
                                Value::RangeType range(v1.toDouble(), v2.toDouble());
                                return Value(range);
                        }
                } else {
        		Value v3 = this->children[2]->evaluate(context);
                        if (v1.type() == Value::NUMBER && v2.type() == Value::NUMBER && v3.type() == Value::NUMBER) {
                                Value::RangeType range(v1.toDouble(), v2.toDouble(), v3.toDouble());
                                return Value(range);
                        }
                }
		return Value();
	}
	if (this->type == "V") {
		Value::VectorType vec;
		BOOST_FOREACH(const Expression *e, this->children) {
			vec.push_back(e->evaluate(context));
		}
		return Value(vec);
	}
	if (this->type == "L")
		return context->lookup_variable(this->var_name);
	if (this->type == "N")
	{
		Value v = this->children[0]->evaluate(context);

		if (v.type() == Value::VECTOR && this->var_name == "x")
			return v[0];
		if (v.type() == Value::VECTOR && this->var_name == "y")
			return v[1];
		if (v.type() == Value::VECTOR && this->var_name == "z")
			return v[2];

		if (v.type() == Value::RANGE && this->var_name == "begin")
			return Value(v[0]);
		if (v.type() == Value::RANGE && this->var_name == "step")
			return Value(v[1]);
		if (v.type() == Value::RANGE && this->var_name == "end")
			return Value(v[2]);

		return Value();
	}
	if (this->type == "F") {
		FuncRecursionGuard g(*this);
		if (g.recursion_detected()) { 
			PRINTB("ERROR: Recursion detected calling function '%s'", this->call_funcname);
			return Value();
		}

		EvalContext c(context, this->call_arguments);
		return context->evaluate_function(this->call_funcname, &c);
	}
	if (this->type == "l") { // let expression
		Context c(context);
		evaluate_sequential_assignment(this->call_arguments, &c);

		return this->children[0]->evaluate(&c);
	}
	if (this->type == "i") { // list comprehension expression
		return this->children[0]->evaluate(context);
	}
	if (this->type == "c") {
		return evaluate_list_comprehension(context);
	}
	abort();
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

std::string Expression::toString() const
{
	std::stringstream stream;

	if (this->type == "*" || this->type == "/" || this->type == "%" || this->type == "+" ||
			this->type == "-" || this->type == "<" || this->type == "<=" || this->type == "==" || 
			this->type == "!=" || this->type == ">=" || this->type == ">" ||
			this->type == "&&" || this->type == "||") {
		stream << "(" << *this->children[0] << " " << this->type << " " << *this->children[1] << ")";
	}
	else if (this->type == "?:") {
		stream << "(" << *this->children[0] << " ? " << *this->children[1] << " : " << *this->children[2] << ")";
	}
	else if (this->type == "[]") {
		stream << *this->children[0] << "[" << *this->children[1] << "]";
	}
	else if (this->type == "I") {
		stream << "-" << *this->children[0];
	}
	else if (this->type == "!") {
		stream << "!" << *this->children[0];
	}
	else if (this->type == "C") {
		stream << this->const_value;
	}
	else if (this->type == "R") {
		stream << "[" << *this->children[0] << " : " << *this->children[1];
		if (this->children.size() > 2) {
			stream << " : " << *this->children[2];
		}
		stream << "]";
	}
	else if (this->type == "V") {
		stream << "[";
		for (size_t i=0; i < this->children.size(); i++) {
			if (i > 0) stream << ", ";
			stream << *this->children[i];
		}
		stream << "]";
	}
	else if (this->type == "L") {
		stream << this->var_name;
	}
	else if (this->type == "N") {
		stream << *this->children[0] << "." << this->var_name;
	}
	else if (this->type == "F") {
		stream << this->call_funcname << "(" << this->call_arguments << ")";
	}
	else if (this->type == "l") {
		stream << "let(" << this->call_arguments << ") " << *this->children[0];
	}
	else if (this->type == "i") { // list comprehension expression
		Expression const* c = this->children[0];

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
		} while (c->type == "c");

		stream << *c << "]";
	}
	else {
		assert(false && "Illegal expression type");
	}

	return stream.str();
}

std::ostream &operator<<(std::ostream &stream, const Expression &expr)
{
	stream << expr.toString();
	return stream;
}

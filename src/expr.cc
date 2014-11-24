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
#include "exceptions.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

ExpressionEvaluator * Expression::evaluators[256];

// static initializer for the expression evaluator lookup table
ExpressionEvaluatorInit Expression::evaluatorInit;

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

class ExpressionEvaluatorAbort : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *, const class Context *) const {
	abort();
    }
};

class ExpressionEvaluatorNot : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return ! expr->children[0]->evaluate(context);
    }
};

class ExpressionEvaluatorLogicalAnd : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) && expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorLogicalOr : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return  expr->children[0]->evaluate(context) || expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorMultiply : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) * expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorDivision : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) / expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorModulo : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) % expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorPlus : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) + expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorMinus : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) - expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorLess : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) < expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorLessOrEqual : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) <= expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorEqual : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) == expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorNotEqual : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children.at(0)->evaluate(context) != expr->children.at(1)->evaluate(context);
    }
};

class ExpressionEvaluatorGreaterOrEqual : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) >= expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorGreater : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context) > expr->children[1]->evaluate(context);
    }
};

class ExpressionEvaluatorTernary : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[expr->children[0]->evaluate(context) ? 1 : 2]->evaluate(context);
    }
};

class ExpressionEvaluatorArray : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context)[expr->children[1]->evaluate(context)];
    }
};

class ExpressionEvaluatorInvert : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return -expr->children[0]->evaluate(context);
    }
};

class ExpressionEvaluatorConst : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *) const {
	return ValuePtr(expr->const_value);
    }
};

class ExpressionEvaluatorRange : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	ValuePtr v1 = expr->children[0]->evaluate(context);
	if (v1->type() == Value::NUMBER) {
		ValuePtr v2 = expr->children[1]->evaluate(context);
		if (v2->type() == Value::NUMBER) {
			if (expr->children.size() == 2) {
				Value::RangeType range(v1->toDouble(), v2->toDouble());
				return ValuePtr(range);
			} else {
				ValuePtr v3 = expr->children[2]->evaluate(context);
				if (v3->type() == Value::NUMBER) {
					Value::RangeType range(v1->toDouble(), v2->toDouble(), v3->toDouble());
					return ValuePtr(range);
				}
			}
		}
	}
	return ValuePtr::undefined;
    }
};

class ExpressionEvaluatorVector : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	Value::VectorType vec;
	BOOST_FOREACH(const Expression *e, expr->children) {
		vec.push_back(*(e->evaluate(context)));
	}
	return ValuePtr(vec);
    }
};

class ExpressionEvaluatorLookup : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return context->lookup_variable(expr->var_name);
    }
};

class ExpressionEvaluatorMember : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	ValuePtr v = expr->children[0]->evaluate(context);

	if (v->type() == Value::VECTOR) {
		if (expr->var_name == "x") return v[0];
		if (expr->var_name == "y") return v[1];
		if (expr->var_name == "z") return v[2];
	} else if (v->type() == Value::RANGE) {
		if (expr->var_name == "begin") return v[0];
		if (expr->var_name == "step") return v[1];
		if (expr->var_name == "end") return v[2];
	}
	return ValuePtr::undefined;
   }
};

class ExpressionEvaluatorFunction : public ExpressionEvaluator
{
	ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
    // FIXME: Throw based on stack usage.
		// NB! This doesn't currently work since the stack usage isn't initialized at this point
    // int su = context->stackUsage();
    // if (su > 1000000) PRINTB("Stack usage: %d", su);
		if (expr->recursioncount >= 1000) {
			throw RecursionException("function", expr->call_funcname.c_str());
			return ValuePtr::undefined;
		}
		expr->recursioncount += 1;
		EvalContext *c = new EvalContext(context, expr->call_arguments);
		ValuePtr result = context->evaluate_function(expr->call_funcname, c);
		delete c;
		expr->recursioncount -= 1;
		return result;
	}
};

class ExpressionEvaluatorLet : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	Context c(context);
	evaluate_sequential_assignment(expr->call_arguments, &c);

	return expr->children[0]->evaluate(&c);
    }
};

class ExpressionEvaluatorLcExpression : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	return expr->children[0]->evaluate(context);
    }
};

class ExpressionEvaluatorLc : public ExpressionEvaluator
{
    ValuePtr evaluate(const class Expression *expr, const class Context *context) const {
	Value::VectorType vec;

	if (expr->call_funcname == "if") {
		if (expr->children[0]->evaluate(context)) {
			if (expr->children[1]->type == EXPRESSION_TYPE_LC) {
				return expr->children[1]->evaluate(context);
			} else {
				vec.push_back((*expr->children[1]->evaluate(context)));
			}
		}
		return ValuePtr(vec);
	} else if (expr->call_funcname == "for") {
		EvalContext for_context(context, expr->call_arguments);

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
					vec.push_back((*expr->children[0]->evaluate(&c)));
				}
			}
		}
		else if (it_values->type() == Value::VECTOR) {
			for (size_t i = 0; i < it_values->toVector().size(); i++) {
				c.set_variable(it_name, it_values->toVector()[i]);
				vec.push_back((*expr->children[0]->evaluate(&c)));
			}
		}
		else if (it_values->type() != Value::UNDEFINED) {
			c.set_variable(it_name, it_values);
			vec.push_back((*expr->children[0]->evaluate(&c)));
		}
		if (expr->children[0]->type == EXPRESSION_TYPE_LC) {
			return ValuePtr(flatten(vec));
		} else {
			return ValuePtr(vec);
		}
	} else if (expr->call_funcname == "let") {
		Context c(context);
		evaluate_sequential_assignment(expr->call_arguments, &c);

		return expr->children[0]->evaluate(&c);
	} else {
		abort();
	}
    }
};

ExpressionEvaluatorInit::ExpressionEvaluatorInit()
{
    ExpressionEvaluator *abort = new ExpressionEvaluatorAbort();
    for (int a = 0;a < 256;a++) {
	Expression::evaluators[a] = abort;
    }
    
    Expression::evaluators[EXPRESSION_TYPE_NOT] = new ExpressionEvaluatorNot();
    Expression::evaluators[EXPRESSION_TYPE_LOGICAL_AND] = new ExpressionEvaluatorLogicalAnd();
    Expression::evaluators[EXPRESSION_TYPE_LOGICAL_OR] = new ExpressionEvaluatorLogicalOr();
    Expression::evaluators[EXPRESSION_TYPE_MULTIPLY] = new ExpressionEvaluatorMultiply();
    Expression::evaluators[EXPRESSION_TYPE_DIVISION] = new ExpressionEvaluatorDivision();
    Expression::evaluators[EXPRESSION_TYPE_MODULO] = new ExpressionEvaluatorModulo();
    Expression::evaluators[EXPRESSION_TYPE_PLUS] = new ExpressionEvaluatorPlus();
    Expression::evaluators[EXPRESSION_TYPE_MINUS] = new ExpressionEvaluatorMinus();
    Expression::evaluators[EXPRESSION_TYPE_LESS] = new ExpressionEvaluatorLess();
    Expression::evaluators[EXPRESSION_TYPE_LESS_OR_EQUAL] = new ExpressionEvaluatorLessOrEqual();
    Expression::evaluators[EXPRESSION_TYPE_EQUAL] = new ExpressionEvaluatorEqual();
    Expression::evaluators[EXPRESSION_TYPE_NOT_EQUAL] = new ExpressionEvaluatorNotEqual();
    Expression::evaluators[EXPRESSION_TYPE_GREATER_OR_EQUAL] = new ExpressionEvaluatorGreaterOrEqual();
    Expression::evaluators[EXPRESSION_TYPE_GREATER] = new ExpressionEvaluatorGreater();
    Expression::evaluators[EXPRESSION_TYPE_TERNARY] = new ExpressionEvaluatorTernary();
    Expression::evaluators[EXPRESSION_TYPE_ARRAY_ACCESS] = new ExpressionEvaluatorArray();
    Expression::evaluators[EXPRESSION_TYPE_INVERT] = new ExpressionEvaluatorInvert();
    Expression::evaluators[EXPRESSION_TYPE_CONST] = new ExpressionEvaluatorConst();
    Expression::evaluators[EXPRESSION_TYPE_RANGE] = new ExpressionEvaluatorRange();
    Expression::evaluators[EXPRESSION_TYPE_VECTOR] = new ExpressionEvaluatorVector();
    Expression::evaluators[EXPRESSION_TYPE_LOOKUP] = new ExpressionEvaluatorLookup();
    Expression::evaluators[EXPRESSION_TYPE_MEMBER] = new ExpressionEvaluatorMember();
    Expression::evaluators[EXPRESSION_TYPE_FUNCTION] = new ExpressionEvaluatorFunction();
    Expression::evaluators[EXPRESSION_TYPE_LET] = new ExpressionEvaluatorLet();
    Expression::evaluators[EXPRESSION_TYPE_LC_EXPRESSION] = new ExpressionEvaluatorLcExpression();
    Expression::evaluators[EXPRESSION_TYPE_LC] = new ExpressionEvaluatorLc();
}

Expression::Expression(const unsigned char type) : recursioncount(0)
{
	setType(type);
}

Expression::Expression(const unsigned char type, Expression *left, Expression *right)
	: recursioncount(0)
{
	setType(type);
	this->children.push_back(left);
	this->children.push_back(right);
}

Expression::Expression(const unsigned char type, Expression *expr)
	: recursioncount(0)
{
	setType(type);
	this->children.push_back(expr);
}

Expression::Expression(const ValuePtr &val) : const_value(val), recursioncount(0)
{
	setType(EXPRESSION_TYPE_CONST);
}

Expression::~Expression()
{
	std::for_each(this->children.begin(), this->children.end(), del_fun<Expression>());
}

void Expression::setType(const unsigned char type)
{
	this->type = type;
	this->evaluator = evaluators[type];
}

ValuePtr Expression::evaluate(const Context *context) const
{
    char _c;
    context->checkStack(&_c);
  
    return evaluator->evaluate(this, context);
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

	switch (this->type) {
	case EXPRESSION_TYPE_MULTIPLY:
	case EXPRESSION_TYPE_DIVISION:
	case EXPRESSION_TYPE_MODULO:
	case EXPRESSION_TYPE_PLUS:
	case EXPRESSION_TYPE_MINUS:
	case EXPRESSION_TYPE_LESS:
	case EXPRESSION_TYPE_GREATER:
		stream << "(" << *this->children[0] << " " << this->type << " " << *this->children[1] << ")";
		break;
	case EXPRESSION_TYPE_NOT:
		stream << "!" << *this->children[0];
		break;
	case EXPRESSION_TYPE_LOGICAL_AND:
		stream << "(" << *this->children[0] << " && " << *this->children[1] << ")";
		break;
	case EXPRESSION_TYPE_LOGICAL_OR:
		stream << "(" << *this->children[0] << " || " << *this->children[1] << ")";
		break;
	case EXPRESSION_TYPE_LESS_OR_EQUAL:
		stream << "(" << *this->children[0] << " <= " << *this->children[1] << ")";
		break;
	case EXPRESSION_TYPE_EQUAL:
		stream << "(" << *this->children[0] << " == " << *this->children[1] << ")";
		break;
	case EXPRESSION_TYPE_NOT_EQUAL:
		stream << "(" << *this->children[0] << " != " << *this->children[1] << ")";
		break;
	case EXPRESSION_TYPE_GREATER_OR_EQUAL:
		stream << "(" << *this->children[0] << " >= " << *this->children[1] << ")";
		break;
	case EXPRESSION_TYPE_TERNARY:
		stream << "(" << *this->children[0] << " ? " << *this->children[1] << " : " << *this->children[2] << ")";
		break;
	case EXPRESSION_TYPE_ARRAY_ACCESS:
		stream << *this->children[0] << "[" << *this->children[1] << "]";
		break;
	case EXPRESSION_TYPE_INVERT:
		stream << "-" << *this->children[0];
		break;
	case EXPRESSION_TYPE_CONST:
		stream << *this->const_value;
		break;
	case EXPRESSION_TYPE_RANGE:
		stream << "[" << *this->children[0] << " : " << *this->children[1];
		if (this->children.size() > 2) {
			stream << " : " << *this->children[2];
		}
		stream << "]";
		break;
	case EXPRESSION_TYPE_VECTOR:
		stream << "[";
		for (size_t i=0; i < this->children.size(); i++) {
			if (i > 0) stream << ", ";
			stream << *this->children[i];
		}
		stream << "]";
		break;
	case EXPRESSION_TYPE_LOOKUP:
		stream << this->var_name;
		break;
	case EXPRESSION_TYPE_MEMBER:
		stream << *this->children[0] << "." << this->var_name;
		break;
	case EXPRESSION_TYPE_FUNCTION:
		stream << this->call_funcname << "(" << this->call_arguments << ")";
		break;
	case EXPRESSION_TYPE_LET:
		stream << "let(" << this->call_arguments << ") " << *this->children[0];
		break;
	case EXPRESSION_TYPE_LC_EXPRESSION:  // list comprehension expression
	//case EXPRESSION_TYPE_LC:
	{
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
		} while (c->type == EXPRESSION_TYPE_LC);

		stream << *c << "]";
		break;
	}
	default:
		assert(false && "Illegal expression type");
		break;
	}
	return stream.str();
}

std::ostream &operator<<(std::ostream &stream, const Expression &expr)
{
	stream << expr.toString();
	return stream;
}

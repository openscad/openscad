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
#include <assert.h>
#include <sstream>
#include <algorithm>
#include "printutils.h"
#include "stackcheck.h"
#include "exceptions.h"
#include "feature.h"
#include "printutils.h"
#include <boost/bind.hpp>

#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

// unnamed namespace
namespace {
	bool isListComprehension(const shared_ptr<Expression> &e) {
		return dynamic_cast<const ListComprehension *>(e.get());
	}
 
	// vec should be a local temporary vector whose values will be moved
	Value::VectorPtr flatten(Value::VectorPtr &vec) {
		int n = 0;
		for (unsigned int i = 0; i < vec->size(); i++) {
			if ((*vec)[i].type() == Value::ValueType::VECTOR) {
				n += (*vec)[i].toVectorPtr()->size();
			} else {
				n++;
			}
		}
		Value::VectorPtr ret; ret->reserve(n);
		for (unsigned int i = 0; i < vec->size(); i++) {
			if (vec[i].type() == Value::ValueType::VECTOR) {
				const Value::VectorPtr &vec_ptr = vec[i].toVectorPtr();
				for(unsigned int j = 0; j < vec_ptr->size(); ++j) {
					ret->emplace_back(std::move(vec_ptr[j]));
				}
			} else {
				ret->emplace_back(std::move(vec[i]));
			}
		}
		return ret;
	}

	void evaluate_sequential_assignment(const AssignmentList &assignment_list, Context *context, const Location &loc) {
		EvalContext ctx(context, assignment_list, loc);
		ctx.assignTo(*context);
	}
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

bool Expression::isLiteral() const
{
    return false;
}

UnaryOp::UnaryOp(UnaryOp::Op op, Expression *expr, const Location &loc) : Expression(loc), op(op), expr(expr)
{
}

Value UnaryOp::evaluate(const Context *context) const
{
	switch (this->op) {
	case (Op::Not):
		return Value(!this->expr->evaluate(context).toBool());
	case (Op::Negate):
		return Value(-this->expr->evaluate(context));
	default:
		return Value();
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
		return "";
		// FIXME: Error: unknown op
	}
}

bool UnaryOp::isLiteral() const { 

    if(this->expr->isLiteral()) 
        return true;
    return false;
}

void UnaryOp::print(std::ostream &stream, const std::string &) const
{
	stream << opString() << *this->expr;
}

BinaryOp::BinaryOp(Expression *left, BinaryOp::Op op, Expression *right, const Location &loc) :
	Expression(loc), op(op), left(left), right(right)
{
}

Value BinaryOp::evaluate(const Context *context) const
{
	switch (this->op) {
	case Op::LogicalAnd:
		return Value(this->left->evaluate(context).toBool() && this->right->evaluate(context).toBool());
		break;
	case Op::LogicalOr:
		return Value(this->left->evaluate(context).toBool() || this->right->evaluate(context).toBool());
		break;
	case Op::Multiply:
		return Value(this->left->evaluate(context) * this->right->evaluate(context));
		break;
	case Op::Divide:
		return Value(this->left->evaluate(context) / this->right->evaluate(context));
		break;
	case Op::Modulo:
		return Value(this->left->evaluate(context) % this->right->evaluate(context));
		break;
	case Op::Plus:
		return Value(this->left->evaluate(context) + this->right->evaluate(context));
		break;
	case Op::Minus:
		return Value(this->left->evaluate(context) - this->right->evaluate(context));
		break;
	case Op::Less:
		return Value(this->left->evaluate(context) < this->right->evaluate(context));
		break;
	case Op::LessEqual:
		return Value(this->left->evaluate(context) <= this->right->evaluate(context));
		break;
	case Op::Greater:
		return Value(this->left->evaluate(context) > this->right->evaluate(context));
		break;
	case Op::GreaterEqual:
		return Value(this->left->evaluate(context) >= this->right->evaluate(context));
		break;
	case Op::Equal:
		return Value(this->left->evaluate(context) == this->right->evaluate(context));
		break;
	case Op::NotEqual:
		return Value(this->left->evaluate(context) != this->right->evaluate(context));
		break;
	default:
		return Value();
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
		return "";
		// FIXME: Error: unknown op
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

Value TernaryOp::evaluate(const Context *context) const
{
	return (this->cond->evaluate(context).toBool() ? this->ifexpr : this->elseexpr)->evaluate(context);
}

void TernaryOp::print(std::ostream &stream, const std::string &) const
{
	stream << "(" << *this->cond << " ? " << *this->ifexpr << " : " << *this->elseexpr << ")";
}

ArrayLookup::ArrayLookup(Expression *array, Expression *index, const Location &loc)
	: Expression(loc), array(array), index(index)
{
}
 
Value ArrayLookup::evaluate(const Context *context) const {
	const Value &array = this->array->evaluate(context);
	return array[this->index->evaluate(context)];
}

void ArrayLookup::print(std::ostream &stream, const std::string &) const
{
	stream << *array << "[" << *index << "]";
}

Literal::Literal(Value val, const Location &loc) : Expression(loc), value(std::move(val))
{
}

Value Literal::evaluate(const class Context *) const
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

Value Range::evaluate(const Context *context) const
{
	Value beginValue = this->begin->evaluate(context);
	if (beginValue.type() == Value::ValueType::NUMBER) {
		Value endValue = this->end->evaluate(context);
		if (endValue.type() == Value::ValueType::NUMBER) {
			if (!this->step) {
				return Value(RangeType(beginValue.toDouble(), endValue.toDouble()));
			} else {
				Value stepValue = this->step->evaluate(context);
				if (stepValue.type() == Value::ValueType::NUMBER) {
					return Value(RangeType(beginValue.toDouble(), stepValue.toDouble(), endValue.toDouble()));
				}
			}
		}
	}
	return Value();
}

void Range::print(std::ostream &stream, const std::string &) const
{
	stream << "[" << *this->begin;
	if (this->step) stream << " : " << *this->step;
	stream << " : " << *this->end;
	stream << "]";
}

bool Range::isLiteral() const {
    if(!this->step){ 
        if( begin->isLiteral() && end->isLiteral())
            return true;
    }else{
        if( begin->isLiteral() && end->isLiteral() && step->isLiteral())
            return true;
    }
    return false;
}

Vector::Vector(const Location &loc) : Expression(loc)
{
}

bool Vector::isLiteral() const {
    for(const auto &e : this->children) {
        if (!e->isLiteral()){
            return false;
        }
    } 
    return true;
}

void Vector::push_back(Expression *expr)
{
	this->children.push_back(shared_ptr<Expression>(expr));
}

Value Vector::evaluate(const Context *context) const
{
	Value::VectorPtr vec;
	for(const auto &e : this->children) {
		Value tmpval = e->evaluate(context);
		if (isListComprehension(e)) {
			const Value::VectorPtr &result = tmpval.toVectorPtr();
			for (size_t i = 0;i < result->size();i++) {
				vec->push_back(std::move(result[i]));
			}
		} else {
			vec->push_back(std::move(tmpval));
		}
	}
	return Value(std::move(vec));
}

void Vector::print(std::ostream &stream, const std::string &) const
{
	stream << "[";
	for (size_t i=0; i < this->children.size(); i++) {
		if (i > 0) stream << ", ";
		stream << *this->children[i];
	}
	stream << "]";
}

Lookup::Lookup(const std::string &name, const Location &loc) : Expression(loc), name(name)
{
}

Value Lookup::evaluate(const Context *context) const
{
	return context->lookup_variable(this->name,false,loc);
}

Value Lookup::evaluateSilently(const Context *context) const
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

Value MemberLookup::evaluate(const Context *context) const
{
	const Value &v = this->expr->evaluate(context);

	if (v.type() == Value::ValueType::VECTOR) {
		if (this->member == "x") return v[0].clone();
		if (this->member == "y") return v[1].clone();
		if (this->member == "z") return v[2].clone();
	} else if (v.type() == Value::ValueType::RANGE) {
		if (this->member == "begin") return v[0].clone();
		if (this->member == "step") return v[1].clone();
		if (this->member == "end") return v[2].clone();
	}
	return Value();
}

void MemberLookup::print(std::ostream &stream, const std::string &) const
{
	stream << *this->expr << "." << this->member;
}

FunctionCall::FunctionCall(const std::string &name, 
													 const AssignmentList &args, const Location &loc)
	: Expression(loc), name(name), arguments(args)
{
}

/**
 * This is separated because PRINTB uses quite a lot of stack space
 * and the method using it evaluate()
 * is called often when recursive functions are evaluated.
 * noinline is required, as we here specifically optimize for stack usage
 * during normal operating, not runtime during error handling.
*/
static void NOINLINE print_err(const char *name, const Location &loc,const Context *ctx){
	std::string locs = loc.toRelativeString(ctx->documentPath());
	PRINTB("ERROR: Recursion detected calling function '%s' %s", name % locs);
}

/**
 * This is separated because PRINTB uses quite a lot of stack space
 * and the method using it evaluate()
 * is called often when recursive functions are evaluated.
 * noinline is required, as we here specifically optimize for stack usage
 * during normal operating, not runtime during error handling.
*/
static void NOINLINE print_trace(const FunctionCall *val, const Context *ctx){
	PRINTB("TRACE: called by '%s', %s.", val->name % val->location().toRelativeString(ctx->documentPath()));
}

Value FunctionCall::evaluate(const Context *context) const
{
	if (StackCheck::inst().check()) {
		print_err(this->name.c_str(),loc,context);
		throw RecursionException::create("function", this->name,this->loc);
	}
	try{
		EvalContext c(context, this->arguments, this->loc);
		Value result = context->evaluate_function(this->name, &c);
		return result;
	}catch(EvaluationException &e){
		if(e.traceDepth>0){
			print_trace(this, context);
			e.traceDepth--;
		}
		throw;
	}
}

void FunctionCall::print(std::ostream &stream, const std::string &) const
{
	stream << this->name << "(" << this->arguments << ")";
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

	// TODO: Generate error/warning if expr != 0?
	return new FunctionCall(funcname, arglist, loc);
}

Assert::Assert(const AssignmentList &args, Expression *expr, const Location &loc)
	: Expression(loc), arguments(args), expr(expr)
{

}

Value Assert::evaluate(const Context *context) const
{
	EvalContext assert_context(context, this->arguments, this->loc);

	Context c(&assert_context);
	evaluate_assert(c, &assert_context);

	return expr ? expr->evaluate(&c) : Value();
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

Value Echo::evaluate(const Context *context) const
{
	EvalContext echo_context(context, this->arguments, this->loc);	
	PRINTB("%s", STR("ECHO: " << echo_context));

	Value result = expr ? expr->evaluate(context) : Value();
	return result;
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

Value Let::evaluate(const Context *context) const
{
	Context c(context);
	evaluate_sequential_assignment(this->arguments, &c, this->loc);

	return this->expr->evaluate(&c);
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

Value LcIf::evaluate(const Context *context) const
{
    const shared_ptr<Expression> &expr = this->cond->evaluate(context).toBool() ? this->ifexpr : this->elseexpr;
	
    Value::VectorPtr vec;
    if (expr) {
        if (isListComprehension(expr)) {
            return expr->evaluate(context);
        } else {
           vec->emplace_back(expr->evaluate(context));
        }
    }

    return Value(std::move(vec));
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

Value LcEach::evaluate(const Context *context) const
{
	Value::VectorPtr vec;

    Value v = this->expr->evaluate(context);

    if (v.type() == Value::ValueType::RANGE) {
        const RangeType &range = v.toRange();
        uint32_t steps = range.numValues();
        if (steps >= 1000000) {
            PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu), %s", steps % loc.toRelativeString(context->documentPath()));
        } else {
            for (RangeType::iterator it = range.begin();it != range.end();it++) {
                vec->emplace_back(*it);
            }
        }
    } else if (v.type() == Value::ValueType::VECTOR) {
        const Value::VectorPtr &vector = v.toVectorPtr();
        for (size_t i = 0; i < vector->size(); i++) {
            vec->emplace_back(std::move(vector[i]));
        }
    } else if (v.type() == Value::ValueType::STRING) {
        utf8_split(v.toString(), [&](Value v) {
            vec->emplace_back(std::move(v));
        });
    } else if (v.type() != Value::ValueType::UNDEFINED) {
        vec->emplace_back(std::move(v));
    }

    if (isListComprehension(this->expr)) {
        return Value(flatten(vec));
    } else {
        return Value(std::move(vec));
    }
}

void LcEach::print(std::ostream &stream, const std::string &) const
{
    stream << "each (" << *this->expr << ")";
}

LcFor::LcFor(const AssignmentList &args, Expression *expr, const Location &loc)
	: ListComprehension(loc), arguments(args), expr(expr)
{
}

Value LcFor::evaluate(const Context *context) const
{
	Value::VectorPtr vec;

    EvalContext for_context(context, this->arguments, this->loc);

    Context assign_context(context);

    // comprehension for statements are by the parser reduced to only contain one single element
    const std::string &it_name = for_context.getArgName(0);
    Value it_values = for_context.getArgValue(0, &assign_context);

    Context c(context);

    if (it_values.type() == Value::ValueType::RANGE) {
        const RangeType &range = it_values.toRange();
        uint32_t steps = range.numValues();
        if (steps >= 1000000) {
            PRINTB("WARNING: Bad range parameter in for statement: too many elements (%lu), %s", steps % loc.toRelativeString(context->documentPath()));
        } else {
            for (RangeType::iterator it = range.begin();it != range.end();it++) {
                c.set_variable(it_name, Value(*it));
                vec->emplace_back(this->expr->evaluate(&c));
            }
        }
    } else if (it_values.type() == Value::ValueType::VECTOR) {
		const Value::VectorPtr &vec2 = it_values.toVectorPtr();
        for (size_t i = 0; i < vec2->size(); i++) {
            c.set_variable(it_name, vec2[i].clone() );
            vec->emplace_back(this->expr->evaluate(&c));
		}
    } else if (it_values.type() == Value::ValueType::STRING) {
        utf8_split(it_values.toString(), [&](Value v) {
            c.set_variable(it_name, v.clone());
            vec->emplace_back(this->expr->evaluate(&c));
        });
    } else if (it_values.type() != Value::ValueType::UNDEFINED) {
        c.set_variable(it_name, it_values.clone());
        vec->emplace_back(this->expr->evaluate(&c));
    }

    if (isListComprehension(this->expr)) {
        return Value(flatten(vec));
    } else {
        return Value(std::move(vec));
    }
}

void LcFor::print(std::ostream &stream, const std::string &) const
{
    stream << "for(" << this->arguments << ") (" << *this->expr << ")";
}

LcForC::LcForC(const AssignmentList &args, const AssignmentList &incrargs, Expression *cond, Expression *expr, const Location &loc)
	: ListComprehension(loc), arguments(args), incr_arguments(incrargs), cond(cond), expr(expr)
{
}

Value LcForC::evaluate(const Context *context) const
{
	Value::VectorPtr vec;

    Context c(context);
    evaluate_sequential_assignment(this->arguments, &c, this->loc);

	unsigned int counter = 0;
    while (this->cond->evaluate(&c).toBool()) {
        vec->emplace_back(this->expr->evaluate(&c));

        if (counter++ == 1000000) {
            std::string locs = loc.toRelativeString(context->documentPath());
            PRINTB("ERROR: for loop counter exceeded limit, %s", locs);
            throw LoopCntException::create("for", loc);
        }

        Context tmp(&c);
        evaluate_sequential_assignment(this->incr_arguments, &tmp, this->loc);
        c.take_variables(tmp);
    }    

    if (isListComprehension(this->expr)) {
        return Value(flatten(vec));
    } else {
        return Value(std::move(vec));
    }
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

Value LcLet::evaluate(const Context *context) const
{
    Context c(context);
    evaluate_sequential_assignment(this->arguments, &c, this->loc);
    return this->expr->evaluate(&c);
}

void LcLet::print(std::ostream &stream, const std::string &) const
{
    stream << "let(" << this->arguments << ") (" << *this->expr << ")";
}

void evaluate_assert(const Context &context, const class EvalContext *evalctx)
{
	AssignmentList args;
	args += Assignment("condition"), Assignment("message");

	Context c(&context);

	AssignmentMap assignments = evalctx->resolveArguments(args, {}, false);
	for (const auto &arg : args) {
		auto it = assignments.find(arg.name);
		if (it != assignments.end()) {
			c.set_variable(arg.name, assignments[arg.name]->evaluate(evalctx));
		}
	}
	
	const Value condition = c.lookup_variable("condition", false, evalctx->loc);

	if (!condition.toBool()) {
		const Expression *expr = assignments["condition"];
		const Value message = c.lookup_variable("message", true);
		
		const auto locs = evalctx->loc.toRelativeString(context.documentPath());
		const auto exprText = expr ? STR(" '" << *expr << "'") : "";
		if (message.isDefined()) {
			PRINTB("ERROR: Assertion%s failed: %s %s", exprText % message.toEchoString() % locs);
		} else {
			PRINTB("ERROR: Assertion%s failed %s", exprText % locs);
		}
		throw AssertionFailedException("Assertion Failed", evalctx->loc);
	}
}

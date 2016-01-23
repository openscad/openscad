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

#include "mathc99.h"
#include "function.h"
#include "function-read.h"
#include "expression.h"
#include "evalcontext.h"
#include "builtin.h"
#include <sstream>
#include <ctime>
#include <limits>
#include <algorithm>
#include "stl-utils.h"
#include "printutils.h"
#include "stackcheck.h"
#include "exceptions.h"
#include <boost/foreach.hpp>

#include "cgalutils.h"
#include <vector>
#include <fstream>

#include <boost/math/special_functions/fpclassify.hpp>
using boost::math::isnan;
using boost::math::isinf;

/*
 Random numbers

 Newer versions of boost/C++ include a non-deterministic random_device and
 auto/bind()s for random function objects, but we are supporting older systems.
*/

#include"boost-utils.h"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
/*Unicode support for string lengths and array accesses*/
#include <glib.h>
// hash double
#include "linalg.h"

FunctionRead::FunctionRead(const char *name, AssignmentList &definition_arguments, Expression *expr)
	: name(name), definition_arguments(definition_arguments), expr(expr)
{
}

FunctionRead::~FunctionRead()
{
	delete expr;
}

ValuePtr FunctionRead::evaluate(const Context *ctx, const EvalContext *evalctx) const
{
	if (!expr) return ValuePtr::undefined;
	Context c(ctx);
	c.setVariables(definition_arguments, evalctx);
	ValuePtr result = expr->evaluate(&c);

	return result;
}

std::string FunctionRead::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	dump << indent << "function " << name << "(";
	for (size_t i=0; i < definition_arguments.size(); i++) {
		const Assignment &arg = definition_arguments[i];
		if (i > 0) dump << ", ";
		dump << arg.first;
		if (arg.second) dump << " = " << *arg.second;
	}
	dump << ") = " << *expr << ";\n";
	return dump.str();
}

class FunctionReadTailRecursion : public FunctionRead
{
private:
	bool invert;
	ExpressionFunctionCall *call; // memory owned by the main expression
	Expression *endexpr; // memory owned by the main expression

public:
	FunctionReadTailRecursion(const char *name, AssignmentList &definition_arguments, Expression *expr, ExpressionFunctionCall *call, Expression *endexpr, bool invert);
	virtual ~FunctionReadTailRecursion();

	virtual ValuePtr evaluate(const Context *ctx, const EvalContext *evalctx) const;
};

FunctionReadTailRecursion::FunctionReadTailRecursion(const char *name, AssignmentList &definition_arguments, Expression *expr, ExpressionFunctionCall *call, Expression *endexpr, bool invert)
	: FunctionRead(name, definition_arguments, expr), invert(invert), call(call), endexpr(endexpr)
{
}

FunctionReadTailRecursion::~FunctionReadTailRecursion()
{
}

ValuePtr FunctionReadTailRecursion::evaluate(const Context *ctx, const EvalContext *evalctx) const
{
	if (!expr) return ValuePtr::undefined;

	Context c(ctx);
	c.setVariables(definition_arguments, evalctx);

	EvalContext ec(&c, call->call_arguments);
	Context tmp(&c);
	unsigned int counter = 0;
	while (invert ^ expr->first->evaluate(&c)) {
		tmp.setVariables(definition_arguments, &ec);
		c.apply_variables(tmp);

		if (counter++ == 1000000) throw RecursionException::create("function", this->name);
	}

	ValuePtr result = endexpr->evaluate(&c);

	return result;
}

FunctionRead * FunctionRead::create(const char *name, AssignmentList &definition_arguments, Expression *expr)
{
	if (dynamic_cast<ExpressionTernary *>(expr)) {
		ExpressionFunctionCall *f1 = dynamic_cast<ExpressionFunctionCall *>(expr->second);
		ExpressionFunctionCall *f2 = dynamic_cast<ExpressionFunctionCall *>(expr->third);
		if (f1 && !f2) {
			if (name == f1->funcname) {
				return new FunctionReadTailRecursion(name, definition_arguments, expr, f1, expr->third, false);
			}
		} else if (f2 && !f1) {
			if (name == f2->funcname) {
				return new FunctionReadTailRecursion(name, definition_arguments, expr, f2, expr->second, true);
			}
		}
	}
	return new FunctionRead(name, definition_arguments, expr);
}

BuiltinFunctionRead::~BuiltinFunctionRead()
{
}

ValuePtr BuiltinFunctionRead::evaluate(const Context *ctx, const EvalContext *evalctx) const
{
	return eval_func(ctx, evalctx);
}

std::string BuiltinFunctionRead::dump(const std::string &indent, const std::string &name) const
{
	std::stringstream dump;
	dump << indent << "builtin function read " << name << "();\n";
	return dump.str();
}

#define QUOTE(x__) # x__
#define QUOTED(x__) QUOTE(x__)

ValuePtr builtin_read_xyz(const Context *, const EvalContext *evalctx)
{
    if (evalctx->numArgs() != 1) {
        PRINT("WARNING: Invalid number of parameters for read_xyz()");
        return ValuePtr::undefined;
    }
    ValuePtr arg0 = evalctx->getArgValue(0);
    if ((arg0->type() != Value::STRING) ) {
        PRINT( "WARNING: Invalid type of parameters for read_xyz()");
        return ValuePtr::undefined;
    }
    std::string filename = arg0->toString();
    std::vector<PointVectorPairK> points;
    std::ifstream in(filename);
    if ( !in ||
        !CGAL::read_xyz_points_and_normals(
            in,std::back_inserter(points),
            CGAL::First_of_pair_property_map<PointVectorPairK>(),
            CGAL::Second_of_pair_property_map<PointVectorPairK>()))
    {
        PRINTB("ERROR: cannot read file %s",filename);
        return ValuePtr::undefined;
    }
    Value::VectorType result;
    Value::VectorType point_vector,normal_vector;
    for ( std::vector<PointVectorPairK>::const_iterator pvp = points.begin(); pvp != points.end(); pvp++ )
    {
        Value::VectorType vec;
        PointK pk=pvp->first;
        for ( int idx = 0; idx<3; idx++ ) {
            vec.push_back(pk[idx]);
        }
        point_vector.push_back(vec);
        Value::VectorType norm;
        VectorK vk=pvp->second;
        for ( int idx = 0; idx<3; idx++ ) {
            norm.push_back(vk[idx]);
        }
        normal_vector.push_back(norm);
    }
    result.push_back(point_vector);
    result.push_back(normal_vector);
    return ValuePtr(result);
}

void register_builtin_read_functions()
{
    Builtins::init("read_xyz", new BuiltinFunctionRead(&builtin_read_xyz));
}

#pragma once

#include "value.h"
#include "typedefs.h"
#include "feature.h"
#include "function.h"

#include <string>
#include <vector>

class BuiltinFunctionRead : public AbstractFunction
{
public:
	typedef ValuePtr (*eval_func_t)(const Context *ctx, const EvalContext *evalctx);
	eval_func_t eval_func;

	BuiltinFunctionRead(eval_func_t f) : eval_func(f) { }
	BuiltinFunctionRead(eval_func_t f, const Feature& feature) : AbstractFunction(feature), eval_func(f) { }
	virtual ~BuiltinFunctionRead();

	virtual ValuePtr evaluate(const Context *ctx, const EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class FunctionRead : public AbstractFunction
{
public:
        std::string name;
	AssignmentList definition_arguments;

	Expression *expr;

	FunctionRead(const char *name, AssignmentList &definition_arguments, Expression *expr);
	virtual ~FunctionRead();

	virtual ValuePtr evaluate(const Context *ctx, const EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
        
        static FunctionRead * create(const char *name, AssignmentList &definition_arguments, Expression *expr);
};

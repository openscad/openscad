#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "value.h"
#include "typedefs.h"
#include <string>
#include <vector>

class AbstractFunction
{
public:
	virtual ~AbstractFunction();
	virtual Value evaluate(const class Context *ctx, const class EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class BuiltinFunction : public AbstractFunction
{
public:
	typedef Value (*eval_func_t)(const Context *ctx, const EvalContext *evalctx);
	eval_func_t eval_func;

	BuiltinFunction(eval_func_t f) : eval_func(f) { }
	virtual ~BuiltinFunction();

	virtual Value evaluate(const Context *ctx, const EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class Function : public AbstractFunction
{
public:
	AssignmentList definition_arguments;

	Expression *expr;

	Function() { }
	virtual ~Function();

	virtual Value evaluate(const Context *ctx, const EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

#endif

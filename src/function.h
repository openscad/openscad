#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "value.h"
#include <string>
#include <vector>

class AbstractFunction
{
public:
	virtual ~AbstractFunction();
	virtual Value evaluate(const class Context *ctx, const std::vector<std::string> &call_argnames, const std::vector<Value> &call_argvalues) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class BuiltinFunction : public AbstractFunction
{
public:
	typedef Value (*eval_func_t)(const Context *ctx, const std::vector<std::string> &argnames, const std::vector<Value> &args);
	eval_func_t eval_func;

	BuiltinFunction(eval_func_t f) : eval_func(f) { }
	virtual ~BuiltinFunction();

	virtual Value evaluate(const Context *ctx, const std::vector<std::string> &call_argnames, const std::vector<Value> &call_argvalues) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class Function : public AbstractFunction
{
public:
	std::vector<std::string> argnames;
	std::vector<class Expression*> argexpr;

	Expression *expr;

	Function() { }
	virtual ~Function();

	virtual Value evaluate(const Context *ctx, const std::vector<std::string> &call_argnames, const std::vector<Value> &call_argvalues) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

#endif

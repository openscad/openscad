#pragma once

#include "value.h"
#include "typedefs.h"
#include "feature.h"

#include <string>
#include <vector>

class AbstractFunction
{
private:
	const Feature *feature;
public:
	AbstractFunction() : feature(NULL) {}
	AbstractFunction(const Feature& feature) : feature(&feature) {}
	virtual ~AbstractFunction();
	virtual bool is_experimental() const { return feature != NULL; }
	virtual bool is_enabled() const { return (feature == NULL) || feature->is_enabled(); }
	virtual ValuePtr evaluate(const class Context *ctx, const class EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class BuiltinFunction : public AbstractFunction
{
public:
	typedef ValuePtr (*eval_func_t)(const Context *ctx, const EvalContext *evalctx);
	eval_func_t eval_func;

	BuiltinFunction(eval_func_t f) : eval_func(f) { }
	BuiltinFunction(eval_func_t f, const Feature& feature) : AbstractFunction(feature), eval_func(f) { }
	virtual ~BuiltinFunction();

	virtual ValuePtr evaluate(const Context *ctx, const EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class Function : public AbstractFunction
{
public:
        std::string name;
	AssignmentList definition_arguments;

	Expression *expr;

	Function(const char *name, AssignmentList &definition_arguments, Expression *expr);
	virtual ~Function();

	virtual ValuePtr evaluate(const Context *ctx, const EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
        
        static Function * create(const char *name, AssignmentList &definition_arguments, Expression *expr);
};

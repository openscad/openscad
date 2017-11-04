#pragma once

#include "AST.h"
#include "value.h"
#include "Assignment.h"
#include "feature.h"

#include <string>
#include <vector>

class AbstractFunction
{
private:
	const Feature *feature;
public:
	AbstractFunction(const Feature &feature) : feature(&feature) {}
	AbstractFunction() : feature(nullptr) {}
	virtual ~AbstractFunction();
	virtual bool is_experimental() const { return feature != nullptr; }
	virtual bool is_enabled() const { return (feature == nullptr) || feature->is_enabled(); }
	virtual ValuePtr evaluate(const class Context *ctx, const class EvalContext *evalctx) const = 0;
	virtual std::string dump(const std::string &indent, const std::string &name) const = 0;
};

class BuiltinFunction : public AbstractFunction
{
public:
	typedef ValuePtr (*eval_func_t)(const Context *ctx, const EvalContext *evalctx);
	eval_func_t eval_func;

	BuiltinFunction(eval_func_t f) : eval_func(f) { }
	BuiltinFunction(eval_func_t f, const Feature &feature) : AbstractFunction(feature), eval_func(f) { }
	virtual ~BuiltinFunction();

	virtual ValuePtr evaluate(const Context *ctx, const EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
};

class UserFunction : public AbstractFunction, public ASTNode
{
public:
	std::string name;
	AssignmentList definition_arguments;

	shared_ptr<Expression> expr;

	UserFunction(const char *name, AssignmentList &definition_arguments, shared_ptr<Expression> expr, const Location &loc);
	virtual ~UserFunction();

	virtual ValuePtr evaluate(const Context *ctx, const EvalContext *evalctx) const;
	virtual std::string dump(const std::string &indent, const std::string &name) const;

	static UserFunction *create(const char *name, AssignmentList &definition_arguments, shared_ptr<Expression> expr, const Location &loc);
};

#pragma once

#include "AST.h"
#include "value.h"
#include "Assignment.h"
#include "feature.h"
#include "evalcontext.h"

#include <string>
#include <vector>

class AbstractFunction
{
private:
	const Feature *feature;
public:
	AbstractFunction(const Feature& feature) : feature(&feature) {}
	AbstractFunction() : feature(nullptr) {}
	virtual ~AbstractFunction();
	virtual bool is_experimental() const { return feature != nullptr; }
	virtual bool is_enabled() const { return (feature == nullptr) || feature->is_enabled(); }
	virtual Value evaluate(const std::shared_ptr<Context>& ctx, const std::shared_ptr<EvalContext>& evalctx) const = 0;
};

class BuiltinFunction : public AbstractFunction
{
public:
	typedef Value (*eval_func_t)(const std::shared_ptr<Context> ctx, const std::shared_ptr<EvalContext> evalctx);
	eval_func_t eval_func;

	BuiltinFunction(eval_func_t f) : eval_func(f) { }
	BuiltinFunction(eval_func_t f, const Feature& feature) : AbstractFunction(feature), eval_func(f) { }
	~BuiltinFunction();

	Value evaluate(const std::shared_ptr<Context>& ctx, const std::shared_ptr<EvalContext>& evalctx) const override;
};

class UserFunction : public AbstractFunction, public ASTNode
{
public:
	std::string name;
	AssignmentList definition_arguments;

	shared_ptr<Expression> expr;

	UserFunction(const char *name, AssignmentList &definition_arguments, shared_ptr<Expression> expr, const Location &loc);
	~UserFunction();

	Value evaluate(const std::shared_ptr<Context>& ctx, const std::shared_ptr<EvalContext>& evalctx) const override;
	void print(std::ostream &stream, const std::string &indent) const override;
};

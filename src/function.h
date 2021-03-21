#pragma once

#include "AST.h"
#include "value.h"
#include "Assignment.h"
#include "feature.h"

#include <string>
#include <vector>

class EvalContext;

class BuiltinFunction
{
public:
	typedef Value (*eval_func_t)(const std::shared_ptr<EvalContext> evalctx);
	eval_func_t evaluate;

private:
	const Feature *feature;

public:
	BuiltinFunction(eval_func_t f) : evaluate(f), feature(nullptr) { }
	BuiltinFunction(eval_func_t f, const Feature& feature) : evaluate(f), feature(&feature) { }

	bool is_experimental() const { return feature != nullptr; }
	bool is_enabled() const { return (feature == nullptr) || feature->is_enabled(); }
};

class UserFunction : public ASTNode
{
public:
	std::string name;
	AssignmentList definition_arguments;
	shared_ptr<Expression> expr;

	UserFunction(const char *name, AssignmentList &definition_arguments, shared_ptr<Expression> expr, const Location &loc);

	void print(std::ostream &stream, const std::string &indent) const override;
};


struct CallableUserFunction
{
	std::shared_ptr<Context> defining_context;
	const UserFunction* function;
};
typedef boost::variant<const BuiltinFunction*, CallableUserFunction, Value, const Value*> CallableFunction;

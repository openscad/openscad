#pragma once

#include <string>

#include "context.h"

class BuiltinContext : public Context
{
public:
	BuiltinContext();
	~BuiltinContext() {}

	ValuePtr evaluate_function(const std::string &name, const class EvalContext *evalctx) const override;
	class AbstractNode *instantiate_module(const class ModuleInstantiation &inst, EvalContext *evalctx) const override;
};

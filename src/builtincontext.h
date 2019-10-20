#pragma once

#include <string>
#include <memory>

#include "context.h"

class BuiltinContext : public Context
{
public:
	~BuiltinContext() {}

	void init() override;
	Value evaluate_function(const std::string &name, const std::shared_ptr<EvalContext>& evalctx) const override;
	class AbstractNode *instantiate_module(const class ModuleInstantiation &inst, const std::shared_ptr<EvalContext>& evalctx) const override;

protected:
	BuiltinContext();

private:
	std::shared_ptr<BuiltinContext> self;

	friend class Context;
};

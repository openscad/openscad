#pragma once

#include "module.h"

class GroupModule : public AbstractModule
{
public:
	GroupModule() { }
	~GroupModule() { }
	class AbstractNode *instantiate(const std::shared_ptr<Context>& ctx, const ModuleInstantiation *inst, const std::shared_ptr<EvalContext>& evalctx = {}) const override;
};

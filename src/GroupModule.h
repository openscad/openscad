#pragma once

#include "module.h"

class GroupModule : public AbstractModule
{
public:
	GroupModule() { }
	virtual ~GroupModule() { }
	virtual class AbstractNode *instantiate(const Context *ctx, const ModuleInstantiation *inst, class EvalContext *evalctx = nullptr) const;
};

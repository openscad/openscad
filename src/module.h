#pragma once

#include <string>
#include "AST.h"
#include "feature.h"

class AbstractModule
{
private:
	const Feature *feature;
public:
	AbstractModule() : feature(nullptr) {}
	AbstractModule(const Feature& feature) : feature(&feature) {}
	virtual ~AbstractModule();
	virtual bool is_experimental() const { return feature != nullptr; }
	virtual bool is_enabled() const { return (feature == nullptr) || feature->is_enabled(); }
	virtual class AbstractNode *instantiate(const std::shared_ptr<class Context>& ctx, const class ModuleInstantiation *inst, const std::shared_ptr<class EvalContext>& evalctx = {}) const = 0;
};

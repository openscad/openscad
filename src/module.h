#pragma once

#include <functional>
#include <string>
#include "AST.h"
#include "feature.h"

class AbstractNode;
class Arguments;
class Children;
class Context;
class ModuleInstantiation;

class AbstractModule
{
private:
	const Feature *feature;
public:
	AbstractModule() : feature(nullptr) {}
	AbstractModule(const Feature& feature) : feature(&feature) {}
	AbstractModule(const Feature* feature) : feature(feature) {}
	virtual ~AbstractModule() {}
	virtual bool is_experimental() const { return feature != nullptr; }
	virtual bool is_enabled() const { return (feature == nullptr) || feature->is_enabled(); }
	virtual AbstractNode* instantiate(const std::shared_ptr<Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<Context>& context) const = 0;
};

class BuiltinModule : public AbstractModule
{
public:
	BuiltinModule(AbstractNode* (*instantiate)(const class ModuleInstantiation *, const std::shared_ptr<Context>&), const Feature* feature = nullptr);
	BuiltinModule(AbstractNode* (*instantiate)(const class ModuleInstantiation *, Arguments, Children), const Feature* feature = nullptr);
	AbstractNode* instantiate(const std::shared_ptr<Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<Context>& context) const override;

private:
	std::function<AbstractNode*(const class ModuleInstantiation *, const std::shared_ptr<class Context>&)> do_instantiate;
};

struct InstantiableModule
{
	std::shared_ptr<Context> defining_context;
	const AbstractModule* module;
};

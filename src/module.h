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
	AbstractModule(const Feature &feature) : feature(&feature) {}
	virtual ~AbstractModule();
	virtual bool is_experimental() const { return feature != nullptr; }
	virtual bool is_enabled() const { return (feature == nullptr) || feature->is_enabled(); }
	virtual class AbstractNode *instantiate(const class Context *ctx, const class ModuleInstantiation *inst, class EvalContext *evalctx = nullptr) const = 0;
	virtual std::string dump(const std::string &indent, const std::string &name) const;
	virtual double lookup_double_variable_with_default(Context &c, std::string variable, double def) const;
	virtual std::string lookup_string_variable_with_default(Context &c, std::string variable, std::string def) const;
};

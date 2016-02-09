#pragma once

#include "typedefs.h"
#include <unordered_map>

class LocalScope
{
public:
	LocalScope();
	~LocalScope();

	size_t numElements() const { return assignments.size() + children.size(); }
	std::string dump(const std::string &indent) const;
	std::vector<class AbstractNode*> instantiateChildren(const class Context *evalctx) const;
	void addChild(ModuleInstantiation *ch);
	void apply(Context &ctx) const;

	AssignmentList assignments;
	ModuleInstantiationList children;
	typedef std::unordered_map<std::string, class AbstractFunction*> FunctionContainer;
	FunctionContainer functions;
	typedef std::unordered_map<std::string, class AbstractModule*> AbstractModuleContainer;
	AbstractModuleContainer	modules;
};

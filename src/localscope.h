#pragma once

#include "Assignment.h"
#include <unordered_map>

class LocalScope
{
public:
	LocalScope();
	~LocalScope();

	size_t numElements() const { return assignments.size() + children.size(); }
	std::string dump(const std::string &indent) const;
	std::vector<class AbstractNode*> instantiateChildren(const class Context *evalctx) const;
	void addChild(class ModuleInstantiation *ch);
	void apply(Context &ctx) const;

	AssignmentList assignments;
	std::vector<ModuleInstantiation*> children;
	typedef std::unordered_map<std::string, class AbstractFunction*> FunctionContainer;
	FunctionContainer functions;
	typedef std::unordered_map<std::string, class AbstractModule*> AbstractModuleContainer;
	AbstractModuleContainer	modules;
};

#pragma once

#include "typedefs.h"
#include <boost/unordered_map.hpp>

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
	typedef boost::unordered_map<std::string, class AbstractFunction*> FunctionContainer;
	FunctionContainer functions;
	typedef boost::unordered_map<std::string, class AbstractModule*> AbstractModuleContainer;
	AbstractModuleContainer	modules;
};

#ifndef LOCALSCOPE_H_
#define LOCALSCOPE_H_

#include "typedefs.h"
#include <boost/unordered_map.hpp>

class LocalScope
{
public:
	LocalScope();
	~LocalScope();

	size_t numElements() const { return assignments.size() + children.size(); }
	std::string dump(const std::string &indent) const;
	std::vector<class AbstractNode*> instantiateChildren(const class Context *evalctx, class FileContext *filectx = NULL) const;
	void addChild(ModuleInstantiation *ch);

	AssignmentList assignments;
  ModuleInstantiationList children;
	typedef boost::unordered_map<std::string, class AbstractFunction*> FunctionContainer;
	FunctionContainer functions;
	typedef boost::unordered_map<std::string, class AbstractModule*> AbstractModuleContainer;
	AbstractModuleContainer	modules;
};

#endif

#include "localscope.h"
#include "modcontext.h"
#include "module.h"
#include "typedefs.h"
#include "expression.h"
#include "function.h"

#include <boost/foreach.hpp>

LocalScope::LocalScope()
{
}

LocalScope::~LocalScope()
{
	BOOST_FOREACH (ModuleInstantiation *v, children) delete v;
	BOOST_FOREACH (FunctionContainer::value_type &f, functions) delete f.second;
	BOOST_FOREACH (AbstractModuleContainer::value_type &m, modules) delete m.second;
}

void LocalScope::addChild(ModuleInstantiation *ch) 
{
	assert(ch != NULL);
	this->children.push_back(ch); 
}

std::string LocalScope::dump(const std::string &indent) const
{
	std::stringstream dump;
	BOOST_FOREACH(const FunctionContainer::value_type &f, this->functions) {
		dump << f.second->dump(indent, f.first);
	}
	BOOST_FOREACH(const AbstractModuleContainer::value_type &m, this->modules) {
		dump << m.second->dump(indent, m.first);
	}
	BOOST_FOREACH(const Assignment &ass, this->assignments) {
		dump << indent << ass.first << " = " << *ass.second << ";\n";
	}
	BOOST_FOREACH(const ModuleInstantiation *inst, this->children) {
		dump << inst->dump(indent);
	}
	return dump.str();
}

std::vector<AbstractNode*> LocalScope::instantiateChildren(const Context *evalctx, FileContext *filectx) const
{
	std::vector<AbstractNode*> childnodes;
	BOOST_FOREACH (ModuleInstantiation *modinst, this->children) {
		AbstractNode *node = modinst->evaluate(filectx ? filectx : evalctx);
		if (node) childnodes.push_back(node);
	}

	return childnodes;
}

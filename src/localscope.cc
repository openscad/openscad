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

// FIXME: Two parameters here is a hack. Rather have separate types of scopes, or check the type of the first parameter. Note const vs. non-const
std::vector<AbstractNode*> LocalScope::instantiateChildren(const Context *evalctx) const
{
	std::vector<AbstractNode*> childnodes;
	BOOST_FOREACH (ModuleInstantiation *modinst, this->children) {
		AbstractNode *node = modinst->evaluate(evalctx);
		if (node) childnodes.push_back(node);
	}

	return childnodes;
}

/*!
	When instantiating a module which can take a scope as parameter (i.e. non-leaf nodes),
	use this method to apply the local scope definitions to the evaluation context.
	This will enable variables defined in local blocks.
	NB! for loops are special as the local block may depend on variables evaluated by the
	for loop parameters. The for loop code will handle this specially.
*/
void LocalScope::apply(Context &ctx) const
{
	BOOST_FOREACH(const Assignment &ass, this->assignments) {
		ctx.set_variable(ass.first, ass.second->evaluate(&ctx));
	}
}

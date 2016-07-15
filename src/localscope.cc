#include "localscope.h"
#include "modcontext.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "expression.h"
#include "function.h"

LocalScope::LocalScope()
{
}

LocalScope::~LocalScope()
{
	for(auto &v : children) delete v;
	for(auto &f : functions) delete f.second;
	for(auto &m : modules) delete m.second;
}

void LocalScope::addChild(ModuleInstantiation *ch) 
{
	assert(ch != NULL);
	this->children.push_back(ch); 
}

std::string LocalScope::dump(const std::string &indent) const
{
	std::stringstream dump;
	for(const auto &f : this->functions) {
		dump << f.second->dump(indent, f.first);
	}
	for(const auto &m : this->modules) {
		dump << m.second->dump(indent, m.first);
	}
	for(const auto &ass : this->assignments) {
		dump << indent << ass.name << " = " << *ass.expr << ";\n";
	}
	for(const auto &inst : this->children) {
		dump << inst->dump(indent);
	}
	return dump.str();
}

std::vector<AbstractNode*> LocalScope::instantiateChildren(const Context *evalctx) const
{
	std::vector<AbstractNode*> childnodes;
	for(const auto &modinst : this->children) {
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
	for(const auto &ass : this->assignments) {
		ctx.set_variable(ass.name, ass.expr->evaluate(&ctx));
	}
}

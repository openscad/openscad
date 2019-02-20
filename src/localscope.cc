#include "localscope.h"
#include "modcontext.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "UserModule.h"
#include "expression.h"
#include "function.h"
#include "annotation.h"
#include "UserModule.h"

LocalScope::LocalScope()
{
}

LocalScope::~LocalScope()
{
	for (auto &v : children) delete v;
	for (auto &f : functions) delete f.second;
	for (auto &m : modules) delete m.second;
}

void LocalScope::addChild(ModuleInstantiation *modinst) 
{
	assert(modinst);
	this->children.push_back(modinst);
}

void LocalScope::addModule(const std::string &name, class UserModule *module)
{
	assert(module);
	this->modules[name] = module;
	this->astModules.push_back({name, module});
}

void LocalScope::addFunction(class UserFunction *func)
{
	assert(func);
	this->functions[func->name] = func;
	this->astFunctions.push_back({func->name, func});
}

void LocalScope::addAssignment(const Assignment &ass)
{
	this->assignments.push_back(ass);
}

void LocalScope::print(std::ostream &stream, const std::string &indent) const
{
	for (const auto &f : this->astFunctions) {
		f.second->print(stream, indent);
	}
	for (const auto &m : this->astModules) {
		m.second->print(stream, indent);
	}
	for (const auto &ass : this->assignments) {
		ass.print(stream, indent);
	}
	for (const auto &inst : this->children) {
		inst->print(stream, indent);
	}
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

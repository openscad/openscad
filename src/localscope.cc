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
}

void LocalScope::addChild(shared_ptr<ASTNode> node)
{
	// FIXME: Move this out of the ASTNode subtype
	if (auto modinst = dynamic_pointer_cast<ModuleInstantiation>(node)) {
		this->addModuleInst(std::move(modinst));
	}
	if (auto module = dynamic_pointer_cast<UserModule>(node)) {
		this->addModule(std::move(module));
	}
	if (auto function = dynamic_pointer_cast<UserFunction>(node)) {
		this->addFunction(std::move(function));
	}
	if (auto assignment = dynamic_pointer_cast<Assignment>(node)) {
		this->addAssignment(std::move(assignment));
	}
	this->children.emplace_back(std::move(node));
}

void LocalScope::addModuleInst(shared_ptr<ModuleInstantiation> &&modinst)
{
	assert(modinst);
	this->children_inst.emplace_back(std::move(modinst));
}

void LocalScope::addModule(shared_ptr<class UserModule> &&module)
{
	assert(module);
	auto it=this->modules.find(module->name);
	if(it!=this->modules.end()) it->second=module;
	else this->modules.emplace(module->name, module);
	this->astModules.emplace_back(module->name, std::move(module));
}

void LocalScope::addFunction(shared_ptr<class UserFunction> &&func)
{
	assert(func);
	auto it=this->functions.find(func->name);
	if(it!=this->functions.end()) it->second=func;
	else this->functions.emplace(func->name, func);
	this->astFunctions.emplace_back(func->name, std::move(func));
}

void LocalScope::addAssignment(shared_ptr<Assignment> &&ass)
{
	this->assignments.emplace_back(std::move(ass));
}

void LocalScope::print(std::ostream &stream, const std::string &indent, const bool inlined) const
{
	for (const auto &f : this->astFunctions) {
		f.second->print(stream, indent);
	}
	for (const auto &m : this->astModules) {
		m.second->print(stream, indent);
	}
	for (const auto &ass : this->assignments) {
		ass->print(stream, indent);
	}
	for (const auto &inst : this->children_inst) {
		inst->print(stream, indent, inlined);
	}
}

std::vector<AbstractNode*> LocalScope::instantiateChildren(const std::shared_ptr<Context> &evalctx) const
{
	std::vector<AbstractNode*> childnodes;
	for(const auto &modinst : this->children_inst) {
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
void LocalScope::apply(const std::shared_ptr<Context> &ctx) const
{
	for(const auto &assignment : this->assignments) {
		ctx->set_variable(assignment->getName(), assignment->getExpr()->evaluate(ctx));
	}
}

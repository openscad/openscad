#include "localscope.h"
#include "modcontext.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "node.h"
#include "UserModule.h"
#include "expression.h"
#include "function.h"
#include "annotation.h"
#include "UserModule.h"

void LocalScope::addModuleInst(const shared_ptr<ModuleInstantiation>& modinst)
{
	assert(modinst);
	this->moduleInstantiations.push_back(modinst);
}

void LocalScope::addModule(const shared_ptr<class UserModule>& module)
{
	assert(module);
	auto it=this->modules.find(module->name);
	if(it!=this->modules.end()) it->second=module;
	else this->modules.emplace(module->name, module);
	this->astModules.emplace_back(module->name, module);
}

void LocalScope::addFunction(const shared_ptr<class UserFunction>& func)
{
	assert(func);
	auto it=this->functions.find(func->name);
	if(it!=this->functions.end()) it->second=func;
	else this->functions.emplace(func->name, func);
	this->astFunctions.emplace_back(func->name, func);
}

void LocalScope::addAssignment(const shared_ptr<Assignment>& ass)
{
	this->assignments.push_back(ass);
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
	for (const auto &inst : this->moduleInstantiations) {
		inst->print(stream, indent, inlined);
	}
}

AbstractNode* LocalScope::instantiateModules(const std::shared_ptr<Context> &context, AbstractNode* target) const
{
	for(const auto &modinst : this->moduleInstantiations) {
		AbstractNode *node = modinst->evaluate(context);
		if (node) {
			target->children.push_back(node);
		}
	}
	return target;
}

AbstractNode* LocalScope::instantiateModules(const std::shared_ptr<Context>& context, AbstractNode* target, const std::vector<size_t>& indices) const
{
	for (size_t index : indices) {
		AbstractNode *node = instantiateModule(context, index);
		if (node) {
			target->children.push_back(node);
		}
	}
	return target;
}

AbstractNode* LocalScope::instantiateModule(const std::shared_ptr<Context>& context, size_t index) const
{
	assert(index < this->moduleInstantiations.size());
	return moduleInstantiations[index]->evaluate(context);
}

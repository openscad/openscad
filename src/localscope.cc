#include "localscope.h"
#include "modcontext.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "UserModule.h"
#include "expression.h"
#include "function.h"
#include "annotation.h"

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

std::string LocalScope::dump(const std::string &indent) const
{
	std::stringstream dump;
	for (const auto &f : this->astFunctions) {
		dump << f.second->dump(indent, f.first);
	}
	for (const auto &m : this->astModules) {
		dump << m.second->dump(indent, m.first);
	}
	for (const auto &ass : this->assignments) {
		if (ass.hasAnnotations()) {
			const Annotation *group = ass.annotation("Group");
			if (group != nullptr) dump << group->dump();
			const Annotation *Description = ass.annotation("Description");
			if (Description != nullptr) dump << Description->dump();
			const Annotation *parameter = ass.annotation("Parameter");
			if (parameter != nullptr) dump << parameter->dump();
		}
		dump << indent << ass.name << " = " << *ass.expr << ";\n";
	}
	for (const auto &inst : this->children) {
		dump << inst->dump(indent);
	}
	return dump.str();
}

std::vector<AbstractNode *> LocalScope::instantiateChildren(const Context *evalctx) const
{
	std::vector<AbstractNode *> childnodes;
	for (const auto &modinst : this->children) {
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
	for (const auto &ass : this->assignments) {
		ctx.set_variable(ass.name, ass.expr->evaluate(&ctx));
	}
}

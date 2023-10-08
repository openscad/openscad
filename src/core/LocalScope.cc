#include "Assignment.h"
#include "LocalScope.h"
#include "ModuleInstantiation.h"
#include "UserModule.h"
#include "function.h"
#include "core/node.h"

void LocalScope::addModuleInst(const shared_ptr<ModuleInstantiation>& modinst)
{
  assert(modinst);
  this->moduleInstantiations.push_back(modinst);
}

void LocalScope::addModule(const shared_ptr<class UserModule>& module)
{
  assert(module);
  auto it = this->modules.find(module->name);
  if (it != this->modules.end()) it->second = module;
  else this->modules.emplace(module->name, module);
  this->astModules.emplace_back(module->name, module);
}

void LocalScope::addFunction(const shared_ptr<class UserFunction>& func)
{
  assert(func);
  auto it = this->functions.find(func->name);
  if (it != this->functions.end()) it->second = func;
  else this->functions.emplace(func->name, func);
  this->astFunctions.emplace_back(func->name, func);
}

void LocalScope::addAssignment(const shared_ptr<Assignment>& assignment)
{
  this->assignments.push_back(assignment);
}

void LocalScope::print(std::ostream& stream, const std::string& indent, const bool inlined) const
{
  for (const auto& f : this->astFunctions) {
    f.second->print(stream, indent);
  }
  for (const auto& m : this->astModules) {
    m.second->print(stream, indent);
  }
  for (const auto& assignment : this->assignments) {
    assignment->print(stream, indent);
  }
  for (const auto& inst : this->moduleInstantiations) {
    inst->print(stream, indent, inlined);
  }
}

std::shared_ptr<AbstractNode> LocalScope::instantiateModules(const std::shared_ptr<const Context>& context, const std::shared_ptr<AbstractNode> &target) const
{
  for (const auto& modinst : this->moduleInstantiations) {
    auto node = modinst->evaluate(context);
    if (node) {
      target->children.push_back(node);
    }
  }
  return target;
}

std::shared_ptr<AbstractNode> LocalScope::instantiateModules(const std::shared_ptr<const Context>& context, const std::shared_ptr<AbstractNode> &target, const std::vector<size_t>& indices) const
{
  for (size_t index : indices) {
    assert(index < this->moduleInstantiations.size());
    auto node = moduleInstantiations[index]->evaluate(context);
    if (node) {
      target->children.push_back(node);
    }
  }
  return target;
}

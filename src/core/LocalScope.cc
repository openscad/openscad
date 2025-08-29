#include "core/LocalScope.h"

#include <cassert>
#include <ostream>
#include <memory>
#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>
#include <iostream>  // coryrc

#include "core/Assignment.h"
#include "core/ModuleInstantiation.h"
#include "core/Namespace.h"
#include "core/UserModule.h"
#include "core/Using.h"
#include "core/function.h"
#include "core/node.h"
#include "utils/exceptions.h"

void LocalScope::addModuleInst(const std::shared_ptr<ModuleInstantiation>& modinst)
{
  assert(modinst);
  this->moduleInstantiations.push_back(modinst);
}

void LocalScope::addModule(const std::shared_ptr<class UserModule>& module)
{
  assert(module);
  auto it = this->modules.find(module->name);
  if (it != this->modules.end()) it->second = module;
  else this->modules.emplace(module->name, module);
  this->astModules.emplace_back(module->name, module);
}

void LocalScope::addFunction(const std::shared_ptr<class UserFunction>& func)
{
  assert(func);
  std::cerr << "addFunction(" << func->name << ")\n";
  auto it = this->functions.find(func->name);
  if (it != this->functions.end()) it->second = func;
  else this->functions.emplace(func->name, func);
  this->astFunctions.emplace_back(func->name, func);
}

void LocalScope::addAssignment(const std::shared_ptr<Assignment>& assignment)
{
  this->assignments.push_back(assignment);
}

void LocalScope::addUsing(const std::shared_ptr<Using>& u)
{
  std::cerr << "addUsing(" << u->getName() << ")\n";
  this->usings.push_back(u->getName());
  this->astNodes.push_back(u);
}

void LocalScope::addNamespace(const std::shared_ptr<Namespace>& ns)
{
  // Namespace contents are already stored elsewhere; this is purely the AST node.
  this->astNodes.push_back(ns);
}

template <typename T>
std::optional<T> LocalScope::lookup(const std::string& name) const
{
  std::cerr << "ERROR lookup<T>(" << name << ")\n";
  return {};
}

template <>
std::optional<UserFunction *> LocalScope::lookup(const std::string& name) const
{
  std::cerr << "lookup<UserFunction*>(" << name << ")\n";
  const auto& search = this->functions.find(name);
  if (search != this->functions.end()) {
    std::cerr << "\tlookup:Found\n";
    return search->second.get();
  }
  std::cerr << "\tlookup:Missing\n";
  return {};
}

template <>
std::optional<UserModule *> LocalScope::lookup(const std::string& name) const
{
  std::cerr << "lookup<UserModule*>(" << name << ")\n";
  const auto& search = this->modules.find(name);
  if (search != this->modules.end()) {
    std::cerr << "\tlookup:Found\n";
    return search->second.get();
  }
  std::cerr << "\tlookup:Missing\n";
  return {};
}

const std::vector<std::string> LocalScope::getUsings() const
{
  std::vector<std::string> ret;
  std::unordered_set<std::string> unique;
  for (const auto name : this->usings) {
    if (auto iter_unique = unique.insert(name); iter_unique.second) {
      ret.push_back(name);
    }
  }
  return ret;
}

void LocalScope::print(std::ostream& stream, const std::string& indent, const bool inlined) const
{
  for (const auto& f : this->astFunctions) {
    f.second->print(stream, indent);
  }
  for (const auto& m : this->astModules) {
    m.second->print(stream, indent);
  }
  for (const auto& a : this->astNodes) {
    a->print(stream, indent);
  }
  for (const auto& assignment : this->assignments) {
    assignment->print(stream, indent);
  }
  for (const auto& inst : this->moduleInstantiations) {
    inst->print(stream, indent, inlined);
  }
}

// For example, the first time this is called, context is a fresh FileContext with parent context of a
// BuiltinContext; context's scope is the root_file's scope; and the target is a fresh RootNode which is
// the top-level evaluated output.
std::shared_ptr<AbstractNode> LocalScope::instantiateModules(
  const std::shared_ptr<const Context>& context, const std::shared_ptr<AbstractNode>& target) const
{
  for (const auto& modinst : this->moduleInstantiations) {
    const auto node = modinst->evaluate(context);
    if (node) {
      target->children.push_back(node);
    }
  }
  return target;
}

std::shared_ptr<AbstractNode> LocalScope::instantiateModules(
  const std::shared_ptr<const Context>& context, const std::shared_ptr<AbstractNode>& target,
  const std::vector<size_t>& indices) const
{
  for (size_t index : indices) {
    assert(index < this->moduleInstantiations.size());
    const auto node = moduleInstantiations[index]->evaluate(context);
    if (node) {
      target->children.push_back(node);
    }
  }
  return target;
}

std::shared_ptr<AbstractNode> LocalNamespaceScope::instantiateModules(
  const std::shared_ptr<const Context>& context, const std::shared_ptr<AbstractNode>& target) const
{
  throw InvalidInstantiationException("Shouldn't be able to instantiate modules in a namespace");
  return NULL;
}

std::shared_ptr<AbstractNode> LocalNamespaceScope::instantiateModules(
  const std::shared_ptr<const Context>& context, const std::shared_ptr<AbstractNode>& target,
  const std::vector<size_t>& indices) const
{
  throw InvalidInstantiationException("Shouldn't be able to instantiate modules in a namespace");
  return NULL;
}

void LocalNamespaceScope::addModuleInst(const std::shared_ptr<class ModuleInstantiation>& modinst)
{
  throw InvalidInstantiationException("Shouldn't be able to instantiate modules in a namespace");
}

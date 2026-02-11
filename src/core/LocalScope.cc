#include "core/LocalScope.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "core/Assignment.h"
#include "core/ModuleInstantiation.h"
#include "core/UserModule.h"
#include "core/function.h"
#include "core/node.h"

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
  auto it = this->functions.find(func->name);
  if (it != this->functions.end()) it->second = func;
  else this->functions.emplace(func->name, func);
  this->astFunctions.emplace_back(func->name, func);
}

void LocalScope::addAssignment(const std::shared_ptr<Assignment>& assignment)
{
  this->assignments.push_back(assignment);
}

template <>
std::optional<UserFunction *> LocalScope::lookup(const std::string& name) const
{
  const auto& search = this->functions.find(name);
  if (search != this->functions.end()) {
    return search->second.get();
  }
  return {};
}

template <>
std::optional<UserModule *> LocalScope::lookup(const std::string& name) const
{
  const auto& search = this->modules.find(name);
  if (search != this->modules.end()) {
    return search->second.get();
  }
  return {};
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

void LocalScope::print_python(std::ostream& stream, std::ostream& stream_def, const std::string& indent,
                              const bool inlined, const int context_mode) const
// 0: array
// 1: return array
// 2: tuple
{
  std::ostringstream tmpstream;
  for (const auto& f : this->astFunctions) {
    f.second->print_python(tmpstream, stream_def, indent);
  }
  for (const auto& m : this->astModules) {
    m.second->print_python(tmpstream, stream_def, indent);
  }
  for (const auto& assignment : this->assignments) {
    assignment->print_python(stream, stream_def, indent);
  }
  if (context_mode == 1) stream << indent << "return ";
  if (this->moduleInstantiations.size() == 1) {
    this->moduleInstantiations[0]->print_python(stream, stream_def, indent, inlined, context_mode);
  } else {
    if (context_mode != 2) stream << "[\n";
    for (size_t i = 0; i < this->moduleInstantiations.size(); i++) {
      if (i > 0) stream << ",\n";
      this->moduleInstantiations[i]->print_python(stream, stream_def, indent + "  ", inlined,
                                                  context_mode);
    }
    if (!inlined) stream << "\n";
    if (context_mode != 2) stream << indent << "]";
  }
  stream_def << tmpstream.str();
}

std::shared_ptr<AbstractNode> LocalScope::instantiateModules(
  const std::shared_ptr<const Context>& context, const std::shared_ptr<AbstractNode>& target) const
{
  for (const auto& modinst : this->moduleInstantiations) {
    const auto node = modinst->evaluate(context);
    if (node) {
      // GroupNode can pass its children through to parent without an implied union.
      // This might later be handled by GeometryEvaluator, but for now just completely
      // remove the GroupNode from the tree.
      std::shared_ptr<GroupNode> gr = std::dynamic_pointer_cast<GroupNode>(node);
      if (gr && !gr->getImpliedUnion()) {
        target->children.insert(target->children.end(), node->children.begin(), node->children.end());
        node->children.clear();
      } else target->children.push_back(node);
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
      // GroupNode can pass its children through to parent without an implied union.
      // This might later be handled by GeometryEvaluator, but for now just completely
      // remove the GroupNode from the tree.
      std::shared_ptr<GroupNode> gr = std::dynamic_pointer_cast<GroupNode>(node);
      if (gr && !gr->getImpliedUnion()) {
        target->children.insert(target->children.end(), node->children.begin(), node->children.end());
        node->children.clear();
      } else target->children.push_back(node);
    }
  }
  return target;
}

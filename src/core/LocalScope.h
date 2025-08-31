#pragma once

#include "core/Assignment.h"
#include <utility>
#include <ostream>
#include <cstddef>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <optional>

class AbstractNode;
class Context;

class LocalScope
{
public:
  size_t numElements() const { return assignments.size() + moduleInstantiations.size(); }
  void print(std::ostream& stream, const std::string& indent, const bool inlined = false) const;
  std::shared_ptr<AbstractNode> instantiateModules(const std::shared_ptr<const Context>& context,
                                                   const std::shared_ptr<AbstractNode>& target) const;
  std::shared_ptr<AbstractNode> instantiateModules(const std::shared_ptr<const Context>& context,
                                                   const std::shared_ptr<AbstractNode>& target,
                                                   const std::vector<size_t>& indices) const;
  void addModuleInst(const std::shared_ptr<class ModuleInstantiation>& modinst);
  void addModule(const std::shared_ptr<class UserModule>& module);
  void addFunction(const std::shared_ptr<class UserFunction>& function);
  void addAssignment(const std::shared_ptr<class Assignment>& assignment);
  bool hasChildren() const { return !(moduleInstantiations.empty()); }

  /**
   * @brief Search corresponding environment of name for type
   *
   * FYI can only find `function x()` not `x = function ()`
   */
  template <typename T>
  std::optional<T> lookup(const std::string& name) const;

  AssignmentList assignments;
  std::vector<std::shared_ptr<ModuleInstantiation>> moduleInstantiations;

private:
  // Modules and functions are stored twice; once for lookup and once for AST serialization
  // FIXME: Should we split this class into an ASTNode and a run-time support class?
  std::unordered_map<std::string, std::shared_ptr<UserFunction>> functions;
  std::unordered_map<std::string, std::shared_ptr<UserModule>> modules;

  // All below only used for printing:
  std::vector<std::pair<std::string, std::shared_ptr<UserModule>>> astModules;
  std::vector<std::pair<std::string, std::shared_ptr<UserFunction>>> astFunctions;
};

template <>
std::optional<UserFunction *> LocalScope::lookup(const std::string& name) const;

template <>
std::optional<UserModule *> LocalScope::lookup(const std::string& name) const;

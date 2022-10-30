#pragma once

#include "Assignment.h"
#include <unordered_map>

class AbstractNode;
class Context;

class LocalScope
{
public:
  size_t numElements() const { return assignments.size() + moduleInstantiations.size(); }
  void print(std::ostream& stream, const std::string& indent, const bool inlined = false) const;
  std::shared_ptr<AbstractNode> instantiateModules(const std::shared_ptr<const Context>& context, const std::shared_ptr<AbstractNode> &target) const;
  std::shared_ptr<AbstractNode> instantiateModules(const std::shared_ptr<const Context>& context, const std::shared_ptr<AbstractNode> &target, const std::vector<size_t>& indices) const;
  void addModuleInst(const shared_ptr<class ModuleInstantiation>& modinst);
  void addModule(const shared_ptr<class UserModule>& module);
  void addFunction(const shared_ptr<class UserFunction>& function);
  void addAssignment(const shared_ptr<class Assignment>& assignment);
  bool hasChildren() const {return !(moduleInstantiations.empty());}

  AssignmentList assignments;
  std::vector<shared_ptr<ModuleInstantiation>> moduleInstantiations;

  // Modules and functions are stored twice; once for lookup and once for AST serialization
  // FIXME: Should we split this class into an ASTNode and a run-time support class?
  std::unordered_map<std::string, shared_ptr<UserFunction>> functions;
  std::vector<std::pair<std::string, shared_ptr<UserFunction>>> astFunctions;

  std::unordered_map<std::string, shared_ptr<UserModule>> modules;
  std::vector<std::pair<std::string, shared_ptr<UserModule>>> astModules;
};

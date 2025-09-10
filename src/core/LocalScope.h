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
  LocalScope() {}
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
  void addUsing(const std::shared_ptr<class Using>& u);
  void addNamespace(const std::shared_ptr<class Namespace>& ns);
  bool hasChildren() const { return !(moduleInstantiations.empty()); }
  const std::string& get_namespace_name() { return ns_name; }

  /**
   * @brief Search corresponding environment of name for type
   *
   * FYI can only find `function x()` not `x = function ()`
   */
  template <typename T>
  std::optional<T> lookup(const std::string& name) const;

  /**
   * @brief Returns list of unique namespace names in reverse order
   *
   * The last using will be first. Repeats get the last using.
   * Like assignments, it takes affect for the whole scope
   */
  const std::vector<std::string> getUsings() const;

  const AssignmentList& getAssignments() const { return assignments; }

  std::vector<std::shared_ptr<ModuleInstantiation>> moduleInstantiations;

private:
  // Modules and functions are stored twice; once for lookup and once for AST serialization
  // FIXME: Should we split this class into an ASTNode and a run-time support class?
  std::unordered_map<std::string, std::shared_ptr<UserFunction>> functions;
  std::unordered_map<std::string, std::shared_ptr<UserModule>> modules;

protected:
  LocalScope(const char *ns_name) : ns_name(std::string(ns_name)) {}
  std::string ns_name;

private:
  /**
   * @brief Non-unique list of namespaces named by `using` in this scope
   */
  std::vector<std::string> usings;

  AssignmentList assignments;

  // All below only used for printing:
  std::vector<std::pair<std::string, std::shared_ptr<UserModule>>> astModules;
  std::vector<std::pair<std::string, std::shared_ptr<UserFunction>>> astFunctions;
  // Cleanup idea: Put functions and modules in astNodes too. The names of them aren't used and the order
  // isn't preserved anyway and there's no deduping for nodes. And, it's only used for printing.
  std::vector<std::shared_ptr<ASTNode>> astNodes;
};

template <>
std::optional<UserFunction *> LocalScope::lookup(const std::string& name) const;

template <>
std::optional<UserModule *> LocalScope::lookup(const std::string& name) const;

class LocalNamespaceScope : public LocalScope
{
public:
  LocalNamespaceScope(const char *name) : LocalScope(name) {}
  std::shared_ptr<AbstractNode> instantiateModules(const std::shared_ptr<const Context>& context,
                                                   const std::shared_ptr<AbstractNode>& target) const;
  std::shared_ptr<AbstractNode> instantiateModules(const std::shared_ptr<const Context>& context,
                                                   const std::shared_ptr<AbstractNode>& target,
                                                   const std::vector<size_t>& indices) const;
  void addModuleInst(const std::shared_ptr<class ModuleInstantiation>& modinst);
};

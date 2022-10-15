#pragma once

#include <string>
#include <vector>

#include "module.h"
#include "LocalScope.h"

class Feature;

class StaticModuleNameStack
{
public:
  StaticModuleNameStack(const std::string& name) {
    stack.push_back(name);
  }
  ~StaticModuleNameStack() {
    stack.pop_back();
  }

  static int size() { return stack.size(); }
  static const std::string& at(int idx) { return stack[idx]; }

private:
  static std::vector<std::string> stack;
};

class UserModule : public AbstractModule, public ASTNode
{
public:
  UserModule(const char *name, const Location& loc, AssignmentList& parameters) :
    ASTNode(loc), name(name), parameters(parameters) { }

  std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const override;
  virtual void print(std::ostream& stream, const std::string& indent) const override;
  static const std::string& stack_element(int n) { return StaticModuleNameStack::at(n); }
  static int stack_size() { return StaticModuleNameStack::size(); }

  std::string name;
  AssignmentList parameters;
  LocalScope body;
};

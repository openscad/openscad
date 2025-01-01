#pragma once

#include <ostream>
#include <memory>
#include <string>
#include <vector>

#include "core/Identifier.h"
#include "core/module.h"
#include "core/LocalScope.h"

class Feature;

class StaticModuleNameStack
{
public:
  StaticModuleNameStack(const Identifier& name) {
    stack.push_back(name);
  }
  ~StaticModuleNameStack() {
    stack.pop_back();
  }

  static int size() { return stack.size(); }
  static const Identifier& at(int idx) { return stack[idx]; }

private:
  static std::vector<Identifier> stack;
};

class UserModule : public AbstractModule, public ASTNode
{
public:
  UserModule(const Identifier &name, const Location& loc) : ASTNode(loc), name(name) { }
  UserModule(const Identifier &name, const Feature& feature, const Location& loc) : AbstractModule(feature), ASTNode(loc), name(name) { }

  std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  static const Identifier& stack_element(int n) { return StaticModuleNameStack::at(n); }
  static int stack_size() { return StaticModuleNameStack::size(); }

  Identifier name;
  AssignmentList parameters;
  LocalScope body;
};

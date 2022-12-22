#pragma once

#include <string>
#include <vector>

#include "module.h"
#include "LocalScope.h"

class StaticModuleNameStack
{
public:
  StaticModuleNameStack(const std::string& name, const std::string &parname) {
    stack_name.push_back(name);
    stack_param.push_back(parname);
  }
  ~StaticModuleNameStack() {
    stack_name.pop_back();
    stack_param.pop_back();
  }

  static int size() { return stack_name.size(); }
  static const std::string& name_at(int idx) { return stack_name[idx]; }
  static const std::string& param_at(int idx) { return stack_param[idx]; }

private:
  static std::vector<std::string> stack_name;
  static std::vector<std::string> stack_param;
};

class UserModule : public AbstractModule, public ASTNode
{
public:
  UserModule(const char *name, const Location& loc) : ASTNode(loc), name(name) { }
  UserModule(const char *name, const class Feature& feature, const Location& loc) : AbstractModule(feature), ASTNode(loc), name(name) { }
  ~UserModule() {}

  std::shared_ptr<AbstractNode> instantiate(const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst, const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  static const std::string& stack_element_name(int n) { return StaticModuleNameStack::name_at(n); }
  static const std::string& stack_element_param(int n) { return StaticModuleNameStack::param_at(n); }
  static int stack_size() { return StaticModuleNameStack::size(); }

  std::string name;
  AssignmentList parameters;
  LocalScope body;
};

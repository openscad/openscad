#pragma once

#include <ostream>
#include <memory>
#include <string>
#include <vector>

#include "core/module.h"
#include "core/AST.h"
#include "core/LocalScope.h"

class Feature;

/**
 * @brief A stack for holding module names while evaluating
 *
 * The names are used for the builtin function `parent_module` and
 * the quantity for calculating `$parent_module` special variable.
 */
class StaticModuleNameStack
{
public:
  /**
   * @brief Push module's name on the static module name stack
   *
   * It will be popped in the destructor.
   */
  StaticModuleNameStack(const std::string& name) { stack.push_back(name); }
  ~StaticModuleNameStack() { stack.pop_back(); }

  static int size() { return stack.size(); }
  static const std::string& at(int idx) { return stack[idx]; }

private:
  static std::vector<std::string> stack;
};

class UserModule : public AbstractModule, public ASTNode
{
public:
  UserModule(const char *name, const Location& loc)
    : ASTNode(loc), name(name), body(std::make_shared<LocalScope>())
  {
  }
  UserModule(const char *name, const Feature& feature, const Location& loc)
    : AbstractModule(feature), ASTNode(loc), name(name), body(std::make_shared<LocalScope>())
  {
  }

  std::shared_ptr<AbstractNode> instantiate(
    const std::shared_ptr<const Context>& defining_context, const ModuleInstantiation *inst,
    const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  static const std::string& stack_element(int n) { return StaticModuleNameStack::at(n); }
  static int stack_size() { return StaticModuleNameStack::size(); }

  std::string name;
  AssignmentList parameters;
  const std::shared_ptr<LocalScope> body;
};

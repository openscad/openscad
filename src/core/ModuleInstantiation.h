#pragma once

#include "core/AST.h"
#include "core/LocalScope.h"
#include <ostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using ModuleInstantiationList = std::vector<class ModuleInstantiation *>;

class ModuleInstantiation : public ASTNode
{
public:
  ModuleInstantiation(std::string name, AssignmentList args = AssignmentList(),
                      const Location& loc = Location::NONE)
    : ASTNode(loc),
      arguments(std::move(args)),
      modname(std::move(name)),
      scope(std::make_shared<LocalScope>())
  {
  }

  ModuleInstantiation(const ModuleInstantiation& ref)
    : ASTNode(ref.loc), arguments(std::move(ref.arguments)), modname(std::move(ref.modname))
  {
  }

  virtual void print(std::ostream& stream, const std::string& indent, const bool inlined) const;
  virtual void print_python(std::ostream& stream, std::ostream& stream_def, const std::string& indent,
                            const bool inlined, const int context_mode) const;
  void print(std::ostream& stream, const std::string& indent) const override
  {
    print(stream, indent, false);
  }
  void print_python(std::ostream& stream, std::ostream& stream_def,
                    const std::string& indent) const override
  {
    print_python(stream, stream_def, indent, false, 0);
  }
  std::shared_ptr<AbstractNode> evaluate(const std::shared_ptr<const Context>& context) const;

  const std::string& name() const { return this->modname; }
  bool isBackground() const { return this->tag_background; }
  bool isHighlight() const { return this->tag_highlight; }
  bool isRoot() const { return this->tag_root; }

  AssignmentList arguments;
  const std::shared_ptr<LocalScope> scope;

  bool tag_root{false};
  bool tag_highlight{false};
  bool tag_background{false};

protected:
  std::string modname;
};

class IfElseModuleInstantiation : public ModuleInstantiation
{
public:
  IfElseModuleInstantiation(std::shared_ptr<class Expression> expr, const Location& loc)
    : ModuleInstantiation("if", AssignmentList{assignment("", std::move(expr))}, loc)
  {
  }

  std::shared_ptr<LocalScope> makeElseScope();
  std::shared_ptr<LocalScope> getElseScope() const { return this->else_scope; }
  void print(std::ostream& stream, const std::string& indent, const bool inlined) const final;
  void print_python(std::ostream& stream, std::ostream& stream_ref, const std::string& indent,
                    const bool inlined, const int context_mode) const final;

private:
  std::shared_ptr<LocalScope> else_scope;
};

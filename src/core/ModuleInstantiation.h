#pragma once

#include "AST.h"
#include "LocalScope.h"
#include <utility>
#include <utility>
#include <vector>
#include "Identifier.h"

using ModuleInstantiationList = std::vector<class ModuleInstantiation *>;

class ModuleInstantiation : public ASTNode
{
public:
  ModuleInstantiation(std::string name, AssignmentList args = AssignmentList(), const Location& loc = Location::NONE)
    : ASTNode(loc), arguments(std::move(args)), modname(std::move(name)) { }

  virtual void print(std::ostream& stream, const std::string& indent, const bool inlined) const;
  void print(std::ostream& stream, const std::string& indent) const override { print(stream, indent, false); }
  std::shared_ptr<AbstractNode> evaluate(const std::shared_ptr<const Context>& context) const;

  const Identifier& name() const { return this->modname; }
  bool isBackground() const { return this->tag_background; }
  bool isHighlight() const { return this->tag_highlight; }
  bool isRoot() const { return this->tag_root; }

  AssignmentList arguments;
  LocalScope scope;

  bool tag_root{false};
  bool tag_highlight{false};
  bool tag_background{false};
protected:
  Identifier modname;
};

class IfElseModuleInstantiation : public ModuleInstantiation
{
public:
  IfElseModuleInstantiation(shared_ptr<class Expression> expr, const Location& loc) :
    ModuleInstantiation("if", AssignmentList{assignment("", std::move(expr))}, loc) { }

  LocalScope *makeElseScope();
  LocalScope *getElseScope() const { return this->else_scope.get(); }
  void print(std::ostream& stream, const std::string& indent, const bool inlined) const final;
private:
  std::unique_ptr<LocalScope> else_scope;
};

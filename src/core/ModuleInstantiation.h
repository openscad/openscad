#pragma once

#include "AST.h"
#include "LocalScope.h"
#include <vector>

typedef std::vector<class ModuleInstantiation *> ModuleInstantiationList;

class ModuleInstantiation : public ASTNode
{
public:
  ModuleInstantiation(std::shared_ptr<Expression> expr, const AssignmentList& args , const Location& loc)
    : ASTNode(loc), arguments(args), tag_root(false), tag_highlight(false), tag_background(false), id_expr(expr) { }
  ModuleInstantiation(std::string const & name, const AssignmentList& args = AssignmentList(), const Location& loc = Location::NONE)
    : ASTNode(loc), arguments(args), tag_root(false), tag_highlight(false), tag_background(false), modname(name) { }
  ~ModuleInstantiation();

  virtual void print(std::ostream& stream, const std::string& indent, const bool inlined) const;
  void print(std::ostream& stream, const std::string& indent) const override { print(stream, indent, false); }
  std::shared_ptr<AbstractNode> evaluate(const std::shared_ptr<const Context> context) const;

  const std::string& name() const { return this->modname; }
  bool isBackground() const { return this->tag_background; }
  bool isHighlight() const { return this->tag_highlight; }
  bool isRoot() const { return this->tag_root; }

  AssignmentList arguments;
  LocalScope scope;

  bool tag_root;
  bool tag_highlight;
  bool tag_background;
protected:
  std::string modname;
  std::shared_ptr<Expression> id_expr;
};

class IfElseModuleInstantiation : public ModuleInstantiation
{
public:
  IfElseModuleInstantiation(shared_ptr<class Expression> expr, const Location& loc) :
    ModuleInstantiation("if", AssignmentList{assignment("", expr)}, loc) { }
  ~IfElseModuleInstantiation();
  LocalScope *makeElseScope();
  LocalScope *getElseScope() const { return this->else_scope.get(); }
  void print(std::ostream& stream, const std::string& indent, const bool inlined) const final;
private:
  std::unique_ptr<LocalScope> else_scope;
};

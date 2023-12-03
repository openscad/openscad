// Several types associated with geometry-as-data.
// NEEDSWORK Should perhaps be split into GeometryLiteral.h
// and GeometryInstantiation.h.  (Or maybe GeometryInstantiation should
// go in ModuleInstantiation.h.)
#pragma once

#include "Expression.h"
#include "Value.h"
#include "LocalScope.h"
#include "Context.h"
#include "AST.h"
#include "ModuleInstantiation.h"

class GeometryLiteral : public Expression
{
public:
  GeometryLiteral(const Location& loc);
  Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
  bool isLiteral() const override;
  LocalScope body;
};

class GeometryInstantiation : public ModuleInstantiation
{
public:
  GeometryInstantiation(shared_ptr<class Expression> expr, const Location& loc);
  virtual std::shared_ptr<AbstractNode> evaluate(const std::shared_ptr<const Context>& context) const override final;
  void print(std::ostream& stream, const std::string& indent, const bool inlined) const final;

private:
  std::shared_ptr<Expression> expr;
};

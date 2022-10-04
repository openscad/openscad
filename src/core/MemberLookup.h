#pragma once

#include <memory>
#include <string>
#include <iostream>
#include "Expression.h"
#include "AST.h"
#include "Context.h"

class MemberLookup : public Expression{
public:
  MemberLookup(Expression *expr, const std::string& member, const Location& loc);
  Value evaluate(const std::shared_ptr<const Context>& context) const override;
  void print(std::ostream& stream, const std::string& indent) const override;
private:
  shared_ptr<Expression> expr;
  std::string member;
};

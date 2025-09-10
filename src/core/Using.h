#pragma once

#include <memory>
#include <ostream>
#include <string>

#include "core/AST.h"

class Using : public ASTNode
{
public:
  Using(std::string ns_name, const Location& loc) : ASTNode(loc), name(std::move(ns_name)) {}

  void print(std::ostream& stream, const std::string& indent) const override;
  const std::string& getName() const { return name; }

protected:
  const std::string name;
};

#pragma once

#include <ostream>
#include <memory>
#include <string>
#include <vector>

#include "core/AST.h"

class LocalScope;

class Namespace : public ASTNode
{
public:
  Namespace(const char *name, std::shared_ptr<LocalScope> body, const Location& loc)
    : ASTNode(loc), name(name), body(body)
  {
  }

  void print(std::ostream& stream, const std::string& indent) const override;

  std::string name;
  std::shared_ptr<LocalScope> body;
};

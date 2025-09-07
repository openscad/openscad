#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "core/AST.h"
#include "core/NamespaceSpecifierList.h"

class NamespaceSpecifier : public ASTNode
{
public:
  NamespaceSpecifier(std::string name, const Location& loc) : ASTNode(loc), name(std::move(name)) {}

  void print(std::ostream& stream, const std::string& indent) const override;
  const std::string& getName() const { return name; }

protected:
  const std::string name;
};

std::ostream& operator<<(std::ostream& stream, const NamespaceSpecifierList& specifiers);

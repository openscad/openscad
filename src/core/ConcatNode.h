#pragma once

#include "node.h"
#include "Value.h"

class ConcatNode : public AbstractPolyNode
{
public:
  VISITABLE();
  ConcatNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "concat"; }
};

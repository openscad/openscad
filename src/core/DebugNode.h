#pragma once

#include "node.h"
#include "Geometry.h"

class DebugNode : public AbstractPolyNode
{
public:
  VISITABLE();
  DebugNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
  }
  std::string toString() const override;
  std::string name() const override { return "debug"; }
  std::vector<int>  faces;
};

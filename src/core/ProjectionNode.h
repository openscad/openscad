#pragma once

#include "node.h"
#include <string>

class ProjectionNode : public AbstractPolyNode
{
public:
  VISITABLE();
  ProjectionNode() : ProjectionNode(new ModuleInstantiation("projection")) { }
  ProjectionNode(ModuleInstantiation *mi) : AbstractPolyNode(mi) { }
  ProjectionNode(bool cut) : ProjectionNode() { this->cut_mode = cut; }
  std::string toString() const override;
  std::string name() const override { return "projection"; }

  int convexity{1};
  bool cut_mode{false};
};

#pragma once

#include "node.h"
#include <string>

class ProjectionNode : public AbstractPolyNode
{
public:
  VISITABLE();
  ProjectionNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) { }
  std::string toString() const override;
  std::string name() const override { return "projection"; }
  std::shared_ptr<AbstractNode> cloneOne() const override;

  int convexity{1};
  bool cut_mode{false};
};

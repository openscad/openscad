#pragma once

#include "node.h"
#include <string>

class RenderNode : public AbstractNode
{
public:
  VISITABLE();
  RenderNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
  std::string toString() const override;
  std::string name() const override { return "render"; }
  std::shared_ptr<AbstractNode> cloneOne() const override;

  int convexity{1};
};

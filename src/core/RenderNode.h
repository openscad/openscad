#pragma once

#include "node.h"
#include <string>

class RenderNode : public AbstractNode
{
public:
  VISITABLE();
  RenderNode(int convexity) : RenderNode() { this->convexity = convexity; }
  RenderNode() : RenderNode(new ModuleInstantiation("render")) { }
  RenderNode(ModuleInstantiation *mi) : AbstractNode(mi) { }
  std::string toString() const override;
  std::string name() const override { return "render"; }

  int convexity{1};
};

#pragma once

#include <string>

#include "core/ModuleInstantiation.h"
#include "core/node.h"

class RenderNode : public AbstractNode
{
public:
  VISITABLE();
  RenderNode(std::shared_ptr<const ModuleInstantiation> mi) : AbstractNode(mi) {}
  std::string toString() const override;
  std::string name() const override { return "render"; }

  int convexity{1};
};

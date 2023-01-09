#pragma once

#include "node.h"
#include <string>

class RenderNode : public AbstractNode
{
public:
  VISITABLE();
  RenderNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
  void print(scad::ostringstream& stream) const override final;
  std::string name() const override final { return "render"; }

  int convexity{1};
};

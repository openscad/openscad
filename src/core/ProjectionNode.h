#pragma once

#include "node.h"
#include <string>

class ProjectionNode : public AbstractPolyNode
{
public:
  VISITABLE();
  ProjectionNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) { }
  void print(scad::ostringstream& stream) const override final;
  std::string name() const override final { return "projection"; }

  int convexity{1};
  bool cut_mode{false};
};

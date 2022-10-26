#pragma once

#include "node.h"
#include <string>

class ProjectionNode : public AbstractPolyNode
{
public:
  VISITABLE();
  ProjectionNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi), convexity(1), cut_mode(false) { }
  void print(scad::ostringstream& stream) const override final;
  std::string name() const override final { return "projection"; }

  int convexity;
  bool cut_mode;
};

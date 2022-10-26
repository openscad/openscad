#pragma once

#include "node.h"
#include "linalg.h"

class ColorNode : public AbstractNode
{
public:
  VISITABLE();
  ColorNode(const ModuleInstantiation *mi) : AbstractNode(mi), color(-1.0f, -1.0f, -1.0f, 1.0f) { }
  void print(scad::ostringstream& stream) const override final;
  std::string name() const override final;

  Color4f color;
};
